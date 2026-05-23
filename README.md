# ESP32-S3 Reconnaissance



## Introduction
An ESP32-S3 that presents itself as both a USB HID keyboard and a USB removable drive simultaneously. When triggered, it types a PowerShell command into the target PC's run dialog, which runs a configuration collection script and writes the results directly back to the ESP32's drive partition.



## How It Works
```
ESP32 (plugged into target PC)
│
├─ USB HID Keyboard → types Win+R, then launches collect.ps1
└─ USB MSC Drive → appears as "ESP32" in Explorer
↑
collect.ps1 writes recon_<date>_<time>.txt in USB MSC Drive (ESP32)
```
1. Plug the ESP32 into the target PC via the USB port.
2. Press the `BOOT button`.
3. The Run dialog opens automatically, PowerShell launches `collect.ps1`.
4. Wait around 30 seconds for collection to complete, then unplug.
5. Plug into your own device, open the `ESP32` drive.



## Prerequisites
#### Hardware Requirements
* `ESP32-S3-DEVKIT-C1 N16R8:` The "N16R8" designation indicates 16MB of Flash memory and 8MB of PSRAM. The custom storage configuration in this project relies entirely on having that 16MB of space to create the hidden USB drive.
* `USB-C Data Cable:` Ensure your cable supports data transfer.
* `Target Machine:` The current payloads are designed for Windows environments, utilizing the Windows run dialog and executing PowerShell scripts.
#### Software Requirements
1.  `Python 3.x:` This project uses custom Python scripts to format the ESP32's internal storage correctly for Windows. 
    * Download from [python.org](https://www.python.org/downloads/). 
    * During installation make sure to check **"Add python.exe to PATH"** before installation.
2.  `Visual Studio Code (VS Code):` Code editor we will use to view the project files and upload the firmware.
    * Download and install from [code.visualstudio.com](https://code.visualstudio.com/).
3.  `PlatformIO IDE:` This is a plug-in that lives inside VS Code. It handles downloading the right board drivers and compiling the C++ code.
    * Open VS Code.
    * Click on the **Extensions** icon on the far-left sidebar.
    * Search for "PlatformIO IDE" and click **Install**. 
> [!TIP]
> [VS Code and PlatformIO Setup](https://www.youtube.com/watch?v=tDh9iNSV2l0)



## Step-by-Step Installation & Setup
Follow these steps to download the project files and open them in your development environment.
### 1. Download the Project Files
* **Method A (Easiest for Beginners):**
  * Scroll to the top of this GitHub page.
  * Click the green **Code** button on the right side.
  * Click **Download ZIP** from the dropdown menu.
  * Once downloaded, locate the ZIP file on your computer and extract its contents into a folder.
* **Method B (For Git Users):**
  If you have Git installed, open your command prompt or terminal and run:
  ```
  git clone https://github.com/Khana-Badosh/ESP32-S3-Reconnaissance.git
  ```
### 2. Open the Project in VS Code & PlatformIO
To ensure PlatformIO configures the board and project files correctly, you must open the project folder using the PlatformIO interface, rather than just opening the files normally in VS Code.
* Open Visual Studio Code.
* Look at the far-left sidebar and click on the PlatformIO icon (it looks like an alien head logo).
* In the PlatformIO Quick Access menu that opens, click on Home, then click Open.
* On the PlatformIO Home screen, click the Open Project button.
* A file browser window will appear. Navigate to the folder where you extracted the project files.
* Select the main project folder (the folder that contains the platformio.ini file) and click Open "...".
* Wait for initialization, look at the bottom status bar of VS Code. Wait for PlatformIO to automatically finish downloading the necessary framework and board files.
* Once initialization finishes, you will see a checkmark or the project name in the bottom status bar, and your environment is fully ready for flashing the board.
### 3. Configuring the COM Port
Before flashing the code or running the scripts, you must ensure that VS Code and PlatformIO are communicating with your specific ESP32-S3 device over the correct serial connection port (known as a **COM port** on Windows).
#### Identify your COM Port
* Plug your ESP32-S3 board into your computer via the COM port.
* Right-click the Windows Start menu button and select **Device Manager**.
* Expand the **Ports (COM & LPT)** section. 
* Look for a device labeled something like `USB Serial Device` or `SERIAL CH343` and note the number next to it (for example, `COM7`).
#### Update the Project Configuration
* In the VS Code file explorer (left side), click on the **`platformio.ini`** file to open it.
* Scroll down until you see the lines for `upload_port` and `monitor_port`.
* Change the value to match your device's actual COM port number. For example, if your device is on COM7:
   ```ini
   upload_port  = COM7
   monitor_port = COM7
   ```
* Press Ctrl + S to save the file.
#### Select the Port in the VS Code Interface
* Look at the very bottom status bar of VS Code.
* Look for the Switch Port icon (it looks like a small plug, a plug with a port name like Auto or COM7).
* Click on it, and a dropdown menu will appear at the top of VS Code. Select your specific COM port from the list or leave it at Auto.
### 4. First-Time Flash
* Click on the PlatformIO icon, open a new terminal via **QUICK ACCESS** > **Miscellaneous** > **New Terminal**.
```bash
python tools/build_fs.py
python tools/flash_fs.py
```
* Flash the firmware
```bash
pio run --target upload
```
> [!IMPORTANT]
> Always flash the filesystem **before** the firmware on a fresh device. Flashing the firmware does not touch the filesystem partition (0x600000+).
### 5. Verifying the Setup (Serial CLI Testing)
Before deploying the device, you can verify that the code flashed correctly and the board is responsive by using the built-in Serial Monitor. 
#### How to Open the Serial Monitor
1. Look at the bottom status bar of VS Code.
2. Click on the **Serial Monitor icon** (it looks like a small plug).
3. The Terminal window at the bottom will open and attempt to connect to your ESP32-S3. 
4. Ensure your terminal's baud rate is set to **`115200`** (PlatformIO configures this automatically from the `platformio.ini` file). 
#### Testing the Commands
Once connected, click inside the terminal window, type any of the following commands, and press **Enter**:
| Command    | Description                      |
| :--------- | :------------------------------- |
| `start`    | Trigger diagnostic sequence      |
| `status`   | Show MSC and diagnostic state    |
| `payloads` | List configured payload commands |
| `help`     | Show all commands                |

If the commands work successfully, your installation was 100% successful! You can now safely unplug your ESP32-S3.



## Understanding the Recon Report (`recon.txt`)
Every time the device successfully completes a run, it saves a detailed text log named using the timestamp format `recon-YYYYMMDD-HHMMSS.txt` inside the internal storage. 
Below is a breakdown of the specific system parameters collected during the sequence:
### 1. Header & Meta Information
* **Timestamp & Context:** Logs the exact date and time the script executed, along with the machine's Hostname, currently active User profile, and Domain group.
### 2. Deep System Information
* **Operating System Details:** Captures the precise Windows OS Name, exact Build Version, Manufacturer, and original Installation Date.
* **Hardware Inventory:** Summarizes vital physical components including:
  * **CPU:** Complete processor model name, total physical cores, and logical thread counts.
  * **RAM Configuration:** Memory capacity, operational speed, and manufacturer tracking codes for each installed stick.
  * **Storage Devices:** Lists all attached drives (including NVMe SSDs, SD cards, and the ESP32's own mass storage allocation) alongside their interface type and hardware serial numbers.
  * **GPU & Baseboard:** Motherboard product version, BIOS release definitions, and active graphics processors with dedicated VRAM limits.
* **System Status:** Identifies Windows activation profiles, system uptime/boot times, and a registry inventory of recently deployed Windows Security Hotfixes.
### 3. Security posture & Defensive Controls
* **Privilege Token Audit:** Details the current user context’s explicit execution privileges (e.g., `SeShutdownPrivilege`, `SeChangeNotifyPrivilege`) and whether they are enabled or disabled.
* **Antivirus Status:** Checks if Windows Defender or third-party AV solutions are functioning, tracking if real-time scanning, antispyware elements, and IOAV protections are actively running.
* **Firewall Configuration:** Scrapes the current operational states and default traffic behaviors for Domain, Private, and Public network firewall profiles.
### 4. Comprehensive Network Topology
* **Network Interfaces (NICs):** Indexes every physical, wireless, and virtual network adapter found on the system. It extracts MAC addresses, active IP addresses (IPv4/IPv6), subnet configurations, and target DHCP servers for:
  * Local Wi-Fi and Bluetooth connections.
  * Hypervisor switches (VirtualBox Host-Only and VMware VMnet adapters).
  * Active VPN endpoints or userspace tunnels (like Tailscale).
* **Active Network Sockets:** Aggregates a listing of all open TCP connection sockets, mapping local ports, remote peer IP addresses, and their current connection states (e.g., `LISTEN`, `ESTABLISHED`).
* **Kernel Routing Tables:** Dumps the host's complete network routing table, detailing destination prefixes, metrics, and interface aliases to outline exactly how the system handles inbound and outbound traffic routing.



## Project Structure
```
ESP32-S3-Reconnaissance/
├── src/
│ ├── main.cpp                           Entry point, setup/loop
│ ├── msc_drive.cpp                      USB Mass Storage drive (FAT16 partition)
│ ├── diagnostic.cpp                     HID keyboard state machine
│ └── cli.cpp                            Serial command-line interface
│
├── include/
│ ├── config.h                           All pins, timing, and MSC identity constants
│ ├── payloads.h                         Run-dialog commands (add new payloads here)
│ ├── keyboard_defs.h                    HID keycodes used by this project
│ ├── msc_drive.h
│ ├── diagnostic.h
│ └── cli.h
│
├── scripts/
│ └── collect.ps1                         PowerShell data collection script
│ [add additional custom scripts here]
│
├── tools/
│ ├── build_fs.py                         Builds FAT16 image from scripts/
│ └── flash_fs.py                         Flashes image to ESP32 at 0x600000
│
├── partitions.csv                        Flash partition layout
└── platformio.ini                        PlatformIO build configuration
```



## Adding a New Payload
1. Write your script and place it in `scripts/`
```
scripts/ network_scan.ps1
```
2. Add a launch command in `include/ payloads.h`
```cpp
static  const  char* payloads[] = {
    // existing entry...

    "powershell -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
    "\"$d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|"
    "select -First 1 -Exp DeviceID);& ($d+'\\network_scan.ps1')\""
};
```
3. Rebuild and flash the filesystem (only needed when scripts change)
```bash
python  tools/build_fs.py
python  tools/flash_fs.py
```
4. Flash the firmware
```bash
pio  run  --target  upload
```

