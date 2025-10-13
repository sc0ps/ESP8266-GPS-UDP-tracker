import os, time, shutil
from SCons.Script import Import

Import("env")

def combine_raw(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    output_dir = os.path.join(project_dir, "firmware")
    os.makedirs(output_dir, exist_ok=True)

    fw = os.path.join(build_dir, "firmware.bin")
    fs = os.path.join(build_dir, "littlefs.bin")
    out = os.path.join(output_dir, "ESP8266_GPS_UDP_Tracker_v1.1.bin")

    print("Combining firmware + filesystem manually with 0x200000 offset...")

    # Als littlefs.bin niet bestaat → build het eerst
    if not os.path.exists(fs):
        print("No filesystem image found — building it now...")
        exit_code = os.system("pio run -t buildfs")
        if exit_code != 0:
            print("Failed to build filesystem image.")
            return
        # wacht tot PlatformIO klaar is met schrijven
        for _ in range(10):
            if os.path.exists(fs):
                break
            time.sleep(0.5)

    if not os.path.exists(fs):
        print("Still no littlefs.bin found after build. Aborting.")
        return

    # Combineer firmware + filesystem met juiste offset
    try:
        with open(fw, "rb") as f_fw, open(fs, "rb") as f_fs, open(out, "wb") as f_out:
            fw_data = f_fw.read()
            f_out.write(fw_data)

            padding = 0x200000 - len(fw_data)
            if padding > 0:
                f_out.write(b"\xFF" * padding)

            f_out.write(f_fs.read())

        print(f"Combined binary created successfully: {out}")
    except Exception as e:
        print(f"Error combining binaries: {e}")

# Run this after the firmware is built
env.AddPostAction("$BUILD_DIR/firmware.bin", combine_raw)
