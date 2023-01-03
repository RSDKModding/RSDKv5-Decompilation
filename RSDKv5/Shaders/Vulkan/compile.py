import os, subprocess, platform

if platform.system() == "Windows":
    PATH_TO_VK_SDK = os.environ['VK_SDK_PATH']
    GLSLC = PATH_TO_VK_SDK + "/bin/glslc.exe"
elif platform.system() == "Linux":
    GLSLC = "glslc"
else:
    print(f"Unsupported OS: {platform.system()}")
    exit(1)


subprocess.call([GLSLC, "",])

try:
    os.mkdir("../CSO-Vulkan")
except: pass

for p in os.scandir("."):
    if not p.name.endswith("py"):
        try:
            subprocess.check_call([GLSLC, p.name, '-DRETRO_REV02=1', '-o', f"../CSO-Vulkan/{p.name}"])
        except subprocess.CalledProcessError as e:
            print("!!! error compiling", p.name)
            # print(e.stderr)
        else:
            print("+++ compiled", p.name)

