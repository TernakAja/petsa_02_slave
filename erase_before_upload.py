import os

def before_upload(source, target, env):
    os.system("esptool.py --port COM3 erase_flash")
