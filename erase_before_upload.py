Import("env")
import os

def before_upload(source, target, env):
    print("Erasing flash before upload...")
    os.system("esptool.py --port {} erase_flash".format(env['UPLOAD_PORT']))

env.AddPreAction("upload", before_upload)
