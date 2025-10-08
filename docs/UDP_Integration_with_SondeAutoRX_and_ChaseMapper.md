# UDP Integration with SondeAutoRX and ChaseMapper (Docker Edition)

This guide explains how to integrate the **GPS UDP Tracker** with an existing **SondeAutoRX** + **ChaseMapper** setup running inside **Docker containers** on a **Raspberry Pi**.  
After following these steps, both applications will automatically use your vehicle’s **live GPS position** through **gpsd**, fed by the **Wemos D1 Mini (ESP8266)** GPS UDP Tracker.

---

## Requirements

- **SondeAutoRX** running in Docker  
- **ChaseMapper** running in Docker  
- **Wemos D1 Mini (ESP8266)** with GPS UDP Tracker firmware installed  
  → sends NMEA sentences (e.g. `$GPGGA`, `$GPRMC`) via **UDP** to the Raspberry Pi (port **5050**)  
- **Raspberry Pi**:
  - Has a working internet connection  
  - Is connected to the **same network and subnet** as the Wemos device

---

## Install gpsd

If gpsd is not yet installed on your Raspberry Pi:

```bash
sudo apt update
sudo apt install gpsd gpsd-clients -y
```

- **gpsd**: the GPS daemon that receives and shares GPS data  
- **gpsd-clients**: utilities such as `cgps`, `gpsmon`, and `gpspipe` for testing  

---

## Configure gpsd

Edit the gpsd configuration file:

```bash
sudo nano /etc/default/gpsd
```

Insert the following configuration:

```bash
START_DAEMON="true"
GPSD_OPTIONS="-n"
DEVICES="udp://0.0.0.0:5050"
USBAUTO="false"
GPSD_SOCKET="/var/run/gpsd.sock"
```

> gpsd will now automatically listen for incoming NMEA data on UDP port **5050**  
> and share it via its default client port **2947** with SondeAutoRX and ChaseMapper.  
> No additional options are required.

Save and restart gpsd:

```bash
sudo systemctl restart gpsd
sudo systemctl status gpsd
```

---

## Verify gpsd is working

Make sure your **Wemos GPS UDP Tracker** is powered on and connected to the same Wi-Fi network.  
Then run:

```bash
cgps -s
```

**If you see coordinates, speed, and time data, gpsd is receiving data correctly.**  
*Note:* This will only appear once your GPS module has a **valid fix** — this can take a few minutes, especially after power-up or when indoors.

To confirm UDP activity:

```bash
sudo netstat -anu | grep 5050
```

Expected output example:
```
udp        0      0 0.0.0.0:5050      0.0.0.0:* 
```

---

## Docker configuration

To allow the containers to access gpsd, make sure your `docker-compose.yml` file uses  
**`network_mode: host`** for each service that needs GPS data.

Example configuration:

```yaml
services:
  radiosonde_auto_rx:
    container_name: radiosonde_auto_rx
    image: ghcr.io/projecthorus/radiosonde_auto_rx:latest
    restart: always
    network_mode: host
    devices:
      - /dev/bus/usb
    volumes:
      - ~/radiosonde_auto_rx/station.cfg:/opt/auto_rx/station.cfg:ro
      - ~/radiosonde_auto_rx/log/:/opt/auto_rx/log/
```

> Note: your container name may differ — replace it with the name used in your setup.

With **host networking**, the container has direct access to gpsd at `localhost:2947`,  
so no extra port mappings or volume mounts are needed.

---

## Configure SondeAutoRX

Edit your `station.cfg` (for example: `~/radiosonde_auto_rx/station.cfg`) and add:

```ini
[location]
# Enter your fixed (home) GPS coordinates as fallback.
# These are only used while the Wemos GPS has not yet obtained a valid fix.
station_lat = 52.0000
station_lon = 4.0000
station_alt = 5.0
gpsd_station_position = true

# Enable gpsd integration
gpsd_enabled = True
gpsd_host = localhost
gpsd_port = 2947
```

> Once gpsd receives a valid fix from the Wemos, these fixed coordinates  
> will automatically be replaced by the live GPS position.

Restart your container (replace `<container_name>` with your actual container name):

```bash
docker restart <container_name>
```

---

## Configure ChaseMapper

Edit the `horusmapper.cfg` file inside your ChaseMapper container and ensure the following section exists:

```ini
[gpsd]
gpsd_host = localhost
gpsd_port = 2947
```

Restart your ChaseMapper container.

---

## Test the integration

| Test | Command | Expected Result |
|------|----------|-----------------|
| gpsd receives data | `cgps -s` | Live coordinates appear (after GPS fix) |
| SondeAutoRX test | Open web interface | “GPSD position active” visible |
| ChaseMapper test | Open `http://<pi-ip>:5001` | Vehicle marker moves live on the map |

---

## GPS Fix Requirement

ChaseMapper will only display your vehicle’s position once the GPS module has achieved a valid 3D fix.

gpsd begins streaming NMEA data as soon as the tracker connects,
but until a 3D fix is obtained, the position values are not considered valid.

In this state, ChaseMapper will show no movement or coordinates on the map.

Once the GPS achieves a 3D fix (latitude, longitude, and altitude lock),
the position immediately becomes visible in both ChaseMapper and AutoRX.

Note:
Depending on environmental conditions, it may take several minutes to obtain a first 3D fix —
especially after power-up, or when operating indoors or under partial sky view.

---

## Troubleshooting

| Problem | Solution |
|----------|-----------|
| `cgps` shows nothing | Ensure the Wemos and Raspberry Pi are on the same subnet and that UDP packets are sent to the correct IP and port 5050 |
| gpsd fails to start | Check `/etc/default/gpsd` for syntax errors |
| SondeAutoRX shows a fixed position only | Verify `gpsd_enabled = True` and `gpsd_station_position = true` |
| ChaseMapper not updating | Check the `[gpsd]` section and connection to port 2947 |
| No UDP traffic detected | Use `sudo netstat -anu` or `tcpdump -i wlan0 udp port 5050` to confirm incoming packets |

---

## Result

After completing these steps, both **SondeAutoRX** and **ChaseMapper** will automatically receive your **live GPS position** through **gpsd**, fed by your **ESP8266 GPS UDP Tracker** — even when running inside Docker! 🚀  

- The Raspberry Pi receives NMEA data via **UDP:5050**  
- gpsd shares the current position on **port 2947**  
- SondeAutoRX and ChaseMapper read this position in real time  

Once your GPS module acquires a **fix**, your position will be updated automatically in both applications.  
Until then, your **fixed fallback location** will be used instead.

