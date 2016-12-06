// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

function send_message(message) {
  var claySetting = JSON.parse(localStorage.getItem('clay-settings'));
  var url = claySetting.host + '?token=' + claySetting.token + '&trigger=' + message;
  console.log('Send webrequest: ' + url);  
  sendGETRequest(url);
}

function sendGETRequest(url) {
  var req = new XMLHttpRequest();
 
  req.onreadystatechange = function(e) {
    if (req.readyState == 4 && req.status == 200) {
      var response = JSON.parse(req.responseText);
      if (response !== undefined && !response.error) {
        console.log("Triggered with event count:" + response.triggered_rules);
        Pebble.sendAppMessage({'EVENTS': response.triggered_rules});
      } else {
        if (response.error){
          console.log("Error message: " + response.error.message);
          Pebble.showSimpleNotificationOnPebble("Trigger failed", response.error.message);
        } else {
          console.log("Unknown response:" + req.responseText);
          Pebble.showSimpleNotificationOnPebble("Trigger failed", "Error on performing trigger");
        }
      }
    } else if (req.status == 404 || req.status == 500) {
      console.log("Error " + req.status);
      Pebble.showSimpleNotificationOnPebble("Trigger failed", "Responsecode: " + req.status);
    }
  };
  
  req.timeout = 4000; // 4 seconds timeout
  req.open("GET", url);
  req.send();
}

Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({'JSREADY': 1});
});

Pebble.addEventListener('appmessage', function(e) {
  var trigger = e.payload.TRIGGER;
  send_message(trigger);
});