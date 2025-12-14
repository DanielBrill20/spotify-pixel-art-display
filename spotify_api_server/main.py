from io import BytesIO
import logging
import os
import socket
import sys
import time
from typing import Any, Dict

from dotenv import find_dotenv, load_dotenv
from PIL import Image, ImageOps
import requests
import spotipy
from spotipy.oauth2 import SpotifyOAuth

TARGET_RESOLUTION = 64  # The physical LED matrix resolution
ART_RESOLUTION = 64  # The desired pixel art resolution, cannot exceed TARGET_RESOLUTION, should also evenly divide TARGET_RESOLUTION
REQUIRED_ENVS = ('CLIENT_ID', 'CLIENT_SECRET', 'REDIRECT_URI')
POLLING_INTERVAL = 0.5
MDNS_HOSTNAME = 'LedMatrix'
POST_TIMEOUT = 5
esp_session = requests.Session()
spotify_session = requests.Session()

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class CoverURL:
    """
    Stores the URL for a given album cover and whether it's larger than the desired size.

    Parameters:
        url (str): The URL for the album image.
        oversized (bool): Whether the image is larger than the ART_RESOLUTION.
    """
    def __init__(self, url: str, oversized: bool):
        self._url = url
        self._oversized = oversized

    @property
    def url(self) -> str:
        return self._url
    
    @property
    def oversized(self) -> bool:
        return self._oversized
    
def resolve_mdns(mdns_hostname: str) -> str:
    """
    Resolves the MCU mDNS hostname to an IP address. Useful to avoid resolving with each HTTP request.

    Parameters:
        mdns_hostname (str): The MCU mDNS hostname to resolve.

    Returns:
        str: The IP address the hostname resolves to.
        If resolution fails, this function simply returns hostname.local.
    """
    try:
        return socket.gethostbyname(f'{mdns_hostname}.local')
    except OSError as e:
        logger.warning(f'Failed to resolve mDNS hostname on init. '
                       f'Using hostname.local (will resolve on connect). '
                       f'Error: {e}')
        return f'{mdns_hostname}.local'
    
def load_and_validate_env() -> Dict[str, str]:
    """
    Loads the .env file and ensures the user has all required environment variables.

    Returns:
        Dict[str, str]: A dictionary of environment variables and their values.

    Raises:
        FileNotFoundError: If .env is not found.
        EnvironmentError: If an environment variable is missing.
    """
    env_path = find_dotenv()
    if not env_path:
        raise FileNotFoundError('.env file not found.')
    load_dotenv(env_path)
    envs = {}
    missing = []
    for key in REQUIRED_ENVS:
        val = os.getenv(key)
        if not val:
            missing.append(key)
        else:
            envs[key] = val.strip()
    if missing:
        raise EnvironmentError(f'Missing required environment vars: {', '.join(missing)}')
    return envs

def auth_spotify() -> Any:
    """
    Authorizes the app to use Spotify with OAuth Authorization Code Flow.
    Explicitly requests permission to read the user's currently playing song.

    Returns:
        Spotify: A Spotify API client from spotipy.
    """
    creds = load_and_validate_env()
    return spotipy.Spotify(auth_manager=SpotifyOAuth(client_id=creds['CLIENT_ID'],
                                                   client_secret=creds['CLIENT_SECRET'],
                                                   redirect_uri=creds['REDIRECT_URI'],
                                                   scope='user-read-currently-playing'))

def get_image_url(album: Any) -> CoverURL:
    """
    Returns a CoverURL object of an album's smallest usable album cover.

    Parameters:
        album (Any): The album portion of Spotify's response to the currently playing endpoint.

    Returns:
        CoverURL: A CoverURL object for the album's album cover.
    """
    images_arr = album['images']  # Images are sorted in decreasing size order
    for image in reversed(images_arr):
        height = image['height']
        if height == ART_RESOLUTION:
            return CoverURL(url=image['url'], oversized=False)
        elif height >= ART_RESOLUTION:
            return CoverURL(url=image['url'], oversized=True)
    # All heights were None (unknown), so take first and resize
    # Or we're using a massive matrix panel lmao
    return CoverURL(url=images_arr[0]['url'], oversized=True)

def get_image_src_bytes(url: str) -> bytes:
    """
    Fetches an image at a given URL.

    Parameters:
        url (str): The image URL.
    
    Returns:
        bytes: The response content (the image bytes).

    Raises:
        HTTPError: If the request fails.
    """
    response = spotify_session.get(url, timeout=3)
    response.raise_for_status()
    return response.content

def generate_byte_arr(cover_url: CoverURL) -> bytes:
    """
    Generates an appropriately sized byte array for an image at a given URL.

    Parameters:
        cover_url (CoverURL): A CoverURL object for the cover image to turn into bytes.

    Returns:
        bytes: The RGB data of the new image, as a byte array.
    """
    src_bytes = get_image_src_bytes(cover_url.url)
    image = Image.open(BytesIO(src_bytes))
    if cover_url.oversized:
        image = image.resize((ART_RESOLUTION, ART_RESOLUTION), resample=Image.Resampling.LANCZOS)
    if ART_RESOLUTION != TARGET_RESOLUTION:
        image = image.resize((TARGET_RESOLUTION, TARGET_RESOLUTION), resample=Image.Resampling.NEAREST)
    if image.mode != 'RGB':
        image = image.convert('RGB')
    # Sometimes the Spotify API claims an image is certain dimensions when in reality it's not
    if image.width != TARGET_RESOLUTION or image.height != TARGET_RESOLUTION:
        logger.warning(
            f'Image was not the right size. '
            f'Expected: {TARGET_RESOLUTION}x{TARGET_RESOLUTION}. '
            f'Actual: {image.width}x{image.height}. Padding image to fit...')
        image = ImageOps.pad(
            image=image,
            size=(TARGET_RESOLUTION, TARGET_RESOLUTION),
            method=Image.Resampling.LANCZOS,
            color=(0, 0, 0))
    return image.tobytes()  # Much faster than image.load() and iterating over pixel data

def send_album_cover(url: str, art_bytes: bytes) -> bool:
    """
    Sends the album cover image bytes to the ESP MCU /image endpoint. Fails gracefully if unsuccessful.

    Parameters:
        url (str): The url of the MCU /image endpoint.
        art_bytes (bytes): The bytes containing the image's RGB data.

    Returns:
        bool: Whether or not the POST request was sent (and received) successfully.
    """
    try:
        resp = esp_session.post(url,
                      data=art_bytes,
                      headers={'Content-Type': 'application/octet-stream'},
                      timeout=POST_TIMEOUT)
        resp.raise_for_status()
    except requests.RequestException as e:
        logger.warning(f'Failed to send image to MCU: {e}')
        return False
    logger.info('Successfully sent image to MCU')
    return True

def send_screensaver_intent(url: str) -> bool:
    """
    Sends an intent to the ESP MCU /screensaver endpoint. Fails gracefully if unsuccessful.

    Parameters:
        url (str): The url of the MCU /screensaver endpoint.

    Returns:
        bool: Whether or not the POST request was sent (and received) successfully.
    """
    try:
        resp = esp_session.post(url,
                      timeout=POST_TIMEOUT)
        resp.raise_for_status()
    except requests.RequestException as e:
        logger.warning(f'Failed to send screensaver intent to MCU: {e}')
        return False
    logger.info('Successfully sent screensaver intent to MCU')
    return True

def snooze(iter_start: float) -> None:
    """
    Stops program execution POLLING_INTERVAL seconds after a given start time.

    Parameters:
        iter_start (float): The program time in which we began counting.
    """
    elapsed = time.perf_counter() - iter_start
    if elapsed < POLLING_INTERVAL:
        time.sleep(POLLING_INTERVAL - elapsed)

def main():
    try:
        spotify_client = auth_spotify()
    except Exception as e:
        # Will catch SpotifyOauthError if OAuth fails, and failures with .env
        logger.critical(f'Startup failed: {e}')
        sys.exit(1)

    mcu_ip = resolve_mdns(MDNS_HOSTNAME)
    image_endpoint = f'http://{mcu_ip}/image'
    screensaver_endpoint = f'http://{mcu_ip}/screensaver'

    current_album_id = None
    screensaver_active = False
    if send_screensaver_intent(screensaver_endpoint):
        screensaver_active = True

    while True:
        try:
            iter_start = time.perf_counter()
            track_response = spotify_client.currently_playing(additional_types='episode')

            if not track_response or not track_response.get('item'):
                current_album_id = None
                if not screensaver_active:
                    if send_screensaver_intent(screensaver_endpoint):
                        screensaver_active = True
                snooze(iter_start)
                continue
            
            # For a song/track, Spotify's response is item > album > id/images
            # For a podcast/episode, Spotify's response is item > id/images
            # So, this allows us to retrieve 'album' art for podcasts too!
            # Optionally, I could look in item > show for a podcast (show is the equivalent of album),
            # but I am choosing to detect when the episode changes in case various episodes have different cover art
            album_obj = track_response['item']
            album = album_obj.get('album') or album_obj

            album_id = album['id']
            if album_id == current_album_id:
                snooze(iter_start)
                continue

            cover_url = get_image_url(album)
            art_bytes = generate_byte_arr(cover_url)
            if send_album_cover(image_endpoint, art_bytes):
                current_album_id = album_id  # Only update after matrix updated
                screensaver_active = False
            snooze(iter_start)
        except Exception as e:
            logger.exception(e)
            time.sleep(3) # Emergency back off

if __name__ == '__main__':
    main()
