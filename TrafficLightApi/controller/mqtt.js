const mqtt    = require('mqtt');

const config  = require('../config.json');

const mqttClient = mqtt.connect(config.mqtt.broker);

var Module = module.exports;
Module.publishEvent = publishEvent;

mqttClient.on('connect', () => {
	console.log('Connected to MQTT!');

	mqttClient.subscribe(config.mqtt.topic_event, function (err) {
		if (err) {
			console.log("MQTT Error: " + err.message);
		}
	});
});

function publishEvent(event) {
    mqttClient.publish(config.mqtt.topic_event, event);
}