Import("env")

def post_program_action(source, target, env):
    print("Program has been built! Creating release files")
    #program_path = target[0].get_abspath()
    #print("Program path", program_path)
    # Copy update binary to ./builds
    env.Execute("cp .pio/build/esp32solo1/firmware.bin ./builds/firmware_update.bin")
    # Create full-size binary into ./builds
    env.Execute('pio pkg exec --package "platformio/tool-esptoolpy" -- esptool.py --chip ESP32 merge_bin -o ./builds/firmware_full.bin --flash_mode dio --flash_size 4MB --fill-flash-size 4MB 0x1000 /home/giuseppe/.platformio/packages/framework-arduino-solo1/tools/sdk/esp32/bin/bootloader_dio_40m.bin 0x8000 .pio/build/esp32solo1/partitions.bin 0xe000 /home/giuseppe/.platformio/packages/framework-arduino-solo1/tools/partitions/boot_app0.bin 0x10000 .pio/build/esp32solo1/firmware.bin')

env.AddPostAction("$PROGPATH", post_program_action)