Import("env")

def before_upload(source, target, env):
    print("Erasing entire flash before upload...")
    # Sisipkan perintah erase_flash
    env.Replace(
        UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADFLAGS erase_flash && '
                  '"$PYTHONEXE" "$UPLOADER" $UPLOADFLAGS $SOURCE"'
    )

env.AddPreAction("upload", before_upload)
