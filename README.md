HAL (Heuristically programmed ALgorithmic computer) is a sentient computer 
(or artificial intelligence) that controls the systems of the UrLab spacecraft 
and interacts with the Hackerspace crew.

# Setup

	$ sudo apt-get install arduino-core libfuse-dev
	$ virtualenv --distribute --no-site-packages ve
	$ source ve/bin/activate
	$ pip install -r requirements.txt

# Compile driver & upload Arduino code
	
	$ make

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
