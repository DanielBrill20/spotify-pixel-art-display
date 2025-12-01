from io import BytesIO
import logging
import os
import sys
import time
from typing import Any, Dict

from dotenv import find_dotenv, load_dotenv
from PIL import Image
import requests
import spotipy
from spotipy.oauth2 import SpotifyOAuth

TARGET_RESOLUTION = 64  # The physical LED matrix resolution
ART_RESOLUTION = 64  # The desired pixel art resolution, cannot exceed TARGET_RESOLUTION, should also evenly divide TARGET_RESOLUTION
REQUIRED_ENVS = ('CLIENT_ID', 'CLIENT_SECRET', 'REDIRECT_URI')
POLLING_INTERVAL = 0.5
MC_HOSTNAME = 'LedMatrix'
IMAGE_ENDPOINT = f'http://{MC_HOSTNAME}.local/image'
SCREENSAVER_ENDPOINT = f'http://{MC_HOSTNAME}.local/screensaver'
POST_TIMEOUT = 7

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
    response = requests.get(url, timeout=3)
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
    return image.tobytes()  # Much faster than image.load() and iterating over pixel data

def send_album_cover(art_bytes: bytes) -> None:
    try:
        requests.post(IMAGE_ENDPOINT,
                      data=art_bytes,
                      headers={'Content-Type': 'application/octet-stream'},
                      timeout=POST_TIMEOUT)
    except:
        logger.warning('Failed to send image to MC')
        return
    logger.info('Successfully sent image to MC')
    return

def send_screensaver_intent() -> None:
    try:
        requests.post(SCREENSAVER_ENDPOINT,
                      timeout=POST_TIMEOUT)
    except:
        logger.warning('Failed to send screensaver intent to MC')
        return
    logger.info('Successfully sent screensaver intent to MC')
    return

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
    current_album_id = None
    send_screensaver_intent()

    while True:
        try:
            iter_start = time.perf_counter()
            track_response = spotify_client.currently_playing(additional_types='episode')

            if not track_response or not track_response.get('item'):
                if current_album_id is not None:
                    current_album_id = None
                    send_screensaver_intent()
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
            send_album_cover(art_bytes)
            current_album_id = album_id  # Only update after matrix updated
            snooze(iter_start)
        except Exception as e:
            logger.exception(e)
            time.sleep(3) # Emergency back off

if __name__ == '__main__':
    main()
