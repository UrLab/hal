HAL (Heuristically programmed ALgorithmic computer) is a sentient computer 
(or artificial intelligence) that controls the systems of the UrLab spacecraft 
and interacts with the Hackerspace crew.

# Setup

	$ git submodule init && git submodule update
	$ sudo apt-get install arduino-core libfuse-dev
	$ virtualenv --distribute --no-site-packages ve
	$ source ve/bin/activate
	$ pip install -r requirements.txt
	$ pushd halpy && python setup.py develop && popd

# Compile driver & upload Arduino code
	
	$ make

# TODO

* Actual Git version (for driver && arduino); not an arbitrary string
* More security check (bound checking, syscall return values, ...)

# Usage
## Launch driver

	$ mkdir halfs
	$ driver/driver halfs

## Access HAL resources

	$ tree -p halfs/
	halfs/
	|-- [dr-xr-xr-x]  animations
	|   |-- [dr-xr-xr-x]  blue
	|   |   |-- [-rw-rw-rw-]  fps
	|   |   |-- [--w--w--w-]  frames
	|   |   |-- [-rw-rw-rw-]  loop
	|   |   `-- [-rw-rw-rw-]  play
	|   |-- [dr-xr-xr-x]  green
	|   |   |-- [-rw-rw-rw-]  fps
	|   |   |-- [--w--w--w-]  frames
	|   |   |-- [-rw-rw-rw-]  loop
	|   |   `-- [-rw-rw-rw-]  play
	|   `-- [dr-xr-xr-x]  red
	|       |-- [-rw-rw-rw-]  fps
	|       |-- [--w--w--w-]  frames
	|       |-- [-rw-rw-rw-]  loop
	|       `-- [-rw-rw-rw-]  play
	|-- [lr--r--r--]  events -> /tmp/hal.sock
	|-- [dr-xr-xr-x]  sensors
	|   |-- [-r--r--r--]  light_inside
	|   |-- [-r--r--r--]  light_outside
	|   |-- [-r--r--r--]  temp_ambiant
	|   `-- [-r--r--r--]  temp_radiator
	|-- [dr-xr-xr-x]  switchs
	|   `-- [-rw-rw-rw-]  power
	|-- [dr-xr-xr-x]  triggers
	|   |-- [-r--r--r--]  bell
	|   |-- [-r--r--r--]  door_stairs
	|   `-- [-r--r--r--]  knife
	`-- [-r--r--r--]  version

	$ cat /dev/hal/version
	0123456789abcdef0123456789abcdef01234567

## Using halpy

In the **script** directory:

```python
from config import get_hal
from halpy.generators import Partition, Note, Silence

hal = get_hal()
hal.sensor('temp_ambiant')

hal.stop('buzzer')
hal.fps('buzzer', 17)
hal.one_shot('buzzer')
hal.upload('buzzer', Partition(Note(440), Silence(), Note(494)).to_frames())
hal.play('buzzer')
```


## Hal Resources
### Switchs 

Switchs are binary outputs (on/off). Just write 1 or 0 to `halfs/switchs/<name>`
to put it on or off. You may also retrieve its actual status by reading the file.


### Animations

Animations are sequences of bytes (0, 1, ... , 254, 255), usually played on 
ledstrips or a buzzer. You can:

* Upload an animation by writing between 1 and 255 bytes to `halfs/animations/<name>/frames`
* Set its speed by writing a number between 4 and 1000 to `halfs/animations/<name>/fps`
* Control if it should play one time or loop continuously by writing 0 or 1 to `halfs/animations/<name>/loop`
* Start or stop it by writing 0 or 1 to `halfs/animations/<name>/play`

You may also get the fps, playing status and loop status by reading corresponding files.

### Sensors

Sensors are analog sensors, varying between 0 and 1 (1024 different values as float).
Just read `halfs/sensors/<name>` to get value

### Triggers

Triggers are binary sensors. Just read `halfs/triggers/<name>` to get value.
You may be also interested in trigger state change in real time, without 
continuously reading the file. You can read the UNIX socket in `halfs/events`,
which outputs a line every time a trigger state changes. This line has the
following format: `<sensor_name>:<status>`, for example `bell:1` or `knife:0`.
