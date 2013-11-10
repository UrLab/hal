HAL (Heuristically programmed ALgorithmic computer) is a sentient computer (or artificial intelligence) that controls the systems of the UrLab spacecraft and interacts with the Hackerspace crew.

# Setup

	$ sudo apt-get install arduino-core
	$ virtualenv --distribute --no-site-packages ve
	$ source ve/bin/activate
	$ pip install -r requirements.txt

# Compile & upload Arduino code
	
	$ pushd arduino && ino build && ino upload; popd

# Communication avec l'Ambianceduino
	
	$ python reader.py #read-only
	$ python daemon.py #Communication

## Interactive
	
	$ python
	>>> from ambianceduino import Ambianceduino
	>>> a = Ambianceduino()
	>>> a.run()  #Démarre le thread de lecture
	>>> a.analogs() #Demande les valeurs des entrées analogiques à l'Ambianceduino
	>>> a.stop() #Ferme le thread de communication

# Programming Ambianceduino
La communication bidirectionnelle avec l'Ambianceduino se fait de manière asynchrone. On envoie des messages à l'arduino en utilisant les méthodes de l'objet Ambianceduino dans le thread principal. 

## Envoi de messages
Les méthodes d'envoi de message sont actuellement:

* on() et off() pour allumer ou éteindre l'alimentation de puissance
* delay(n) pour mettre la vitesse de l'animation des leds à 1000/n FPS. Appelée sans argument, demande seulement la valeur du délai actuel.
* analogs() pour demander l'état des entrées analogiques
* upload_anim(animation) envoie l'animation des LEDs à jouer. Une animation est constituée de 1 à 255 nombres de 8 bits non signés (de 0 à 255). La fonction attend une séquence d'entiers comme paramètre.

## Réception de messages

Les messages sont récupérés dans un thread séparé. Pour lancer le thread de lecture, il faut appeler la methode run() d'un AmbianceduinoReader. Pour l'arrêter, il faut utiliser la méthode stop().

Pour récupérer les informations envoyées par l'ambianceduino, il faut utiliser les callbacks de la classe AmbianceduinoReader. Ces callbacks sont toutes les méthodes dont le nom commence par when_. Ceux-ci sont:
 * when_on(), when_off() appelées lorsque l'arduino confirme qu'il a allumé ou éteint l'alimentation de puissance
 * when_delay(n) appelé lorsque l'arduino envoie le délai actuel de l'animation des leds
 * when_anim(n) appelée lorsqu'une animation de leds a bien été reçue par l'arduino (paramètre: longueur de  l'animation reçue)
 * when_bell() appelée lorsque la sonnette est pressée
 * when_door() appelée lorsque la porte des escaliers est ouverte
 * when_radiator() appelée lorsque la vanne du radiateur est ouverte et que l'alimentation est éteinte (le radiateur est allumé mais le hackerspace est fermé)
 * when_analogs(v) appelée lorsque les valeurs des entrées analogiques sont envoyées (paramètre: dictionnaire des entrées analogiques)
 * when_error(m) appelée lorsqu'un message d'erreur est reçu (paramètre: message)
