from io import BytesIO
import logging
import os
import time
from typing import Any

from dotenv import load_dotenv
from PIL import Image
import requests
import spotipy
from spotipy.oauth2 import SpotifyOAuth

TARGET_RESOLUTION = 64 # The physical LED matrix resolution
ART_RESOLUTION = 64 # The desired pixel art resolution, cannot exceed TARGET_RESOLUTION, should also evenly divide TARGET_RESOLUTION

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

def auth_spotify() -> Any:
    """
    Authorizes the app to use Spotify with OAuth Authorization Code Flow.
    Explicitly requests permission to read the user's currently playing song.

    Returns:
        Spotify: A Spotify API client from spotipy.
    """
    return spotipy.Spotify(auth_manager=SpotifyOAuth(client_id=os.getenv('CLIENT_ID'),
                                                   client_secret=os.getenv('CLIENT_SECRET'),
                                                   redirect_uri=os.getenv('REDIRECT_URI'),
                                                   scope='user-read-currently-playing'))

def get_image_url(album: Any) -> CoverURL:
    """
    Returns an CoverURL object of an album's smallest usable album cover.

    Parameters:
        album (Any): The album portion of Spotify's response to the currently playing endpoint.

    Returns:
        CoverURL: An CoverURL object for the album's album cover.
    """
    images_arr = album['images'] # Images are sorted in decreasing size order
    for image in reversed(images_arr):
        height = image['height']
        if height == ART_RESOLUTION:
            return CoverURL(url=image['url'], oversized=False)
        elif height >= ART_RESOLUTION:
            return CoverURL(url=image['url'], oversized=True)
    else:
        # All heights were None (unknown), so take first and resize
        # Or we're using a massive matrix panel lmao
        return CoverURL(url=images_arr[0]['url'], oversized=True)

def generate_byte_arr(cover_url: CoverURL) -> bytes:
    """
    Generates an appropriately sized byte array for an image at a given URL.

    Parameters:
        cover_url (CoverURL): A CoverURL object for the cover image to turn into bytes.

    Returns:
        bytes: The RGB data of the new image, as a byte array.
    """
    src_bytes = requests.get(cover_url.url).content
    image = Image.open(BytesIO(src_bytes))
    if cover_url.oversized:
        image = image.resize((ART_RESOLUTION, ART_RESOLUTION), resample=Image.Resampling.LANCZOS)
    if ART_RESOLUTION != TARGET_RESOLUTION:
        image = image.resize((TARGET_RESOLUTION, TARGET_RESOLUTION), resample=Image.Resampling.NEAREST)
    if image.mode != 'RGB':
        image = image.convert('RGB')
    return image.tobytes() # Much faster than image.load() and iterating over pixel data

def send_album_cover(art_bytes: bytes) -> None:
    # TODO: Send ESP image bytes to display
    return

def send_screensaver_intent() -> None:
    # TODO: Tell ESP to switch to screensaver mode
    return

def snooze(iter_start: float) -> None:
    """
    Stops program execution 0.5 seconds after a given start time.

    Parameters:
        iter_start (float): The program time in which we began counting.
    """
    elapsed = time.perf_counter() - iter_start
    if elapsed < 0.5:
        time.sleep(0.5 - elapsed)

def main():
    load_dotenv()

    spotify_client = auth_spotify()
    current_album_id = None

    while True:
        try:
            iter_start = time.perf_counter()
            track_response = spotify_client.currently_playing(additional_types='episode')

            if not track_response or not track_response['item']:
                if current_album_id is not None:
                    current_album_id = None
                    send_screensaver_intent()
                    snooze(iter_start)
                continue
            
            # For a song/track, Spotify's response is item > album > id/images
            # For a podcast/episode, Spotify's response is item > id/images
            # So, this allows us to retreive 'album' art for podcasts too!
            # Optionally, I could look in item > show for a podcast (show is the equivalent of album),
            # but I am choosing to detect when the episode changes in case various episodes have different cover art
            album_obj = track_response['item']
            album = album_obj.get('album') or album_obj

            album_id = album['id']
            if album_id == current_album_id:
                snooze(iter_start)
                continue
            current_album_id = album_id

            cover_url = get_image_url(album)
            art_bytes = generate_byte_arr(cover_url)
            send_album_cover(art_bytes)
            snooze(iter_start)
        except Exception as e:
            logger.error(e)
            time.sleep(3) # Emergency back off

if __name__ == '__main__':
    main()
