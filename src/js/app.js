/****
/*Getting battery level part is from source of 11weeks watchface
/*Copyright (c) 2015 Programus
/*https://github.com/programus/pebble-watchface-11weeks/blob/master/LICENSE
*/

var initialized = false;

var battery;
var CHARGING_MASK =           0x80;
var LEVEL_MASK =              0x7f;
var BATTERY_API_UNSUPPORTED = 0x70;

var getBatteryStateInt = function () {
  'use strict';
  var state = 0;
  console.log("battery is: " + battery);
  if (battery) {
    state = Math.round(battery.level * 100);
    if (battery.charging) {
      state |= CHARGING_MASK;
    }
    console.log('got battery state: ' + state);
  } else {
    state = BATTERY_API_UNSUPPORTED;
    console.log('battery API unsupported.');
  }
  return state;
};

var sendBatteryState = function () {
  'use strict';
  var dict = {
      "KEY_PHONE_BATTERY": getBatteryStateInt()
  };
  Pebble.sendAppMessage(dict);
  console.log('battery state sent: ' + JSON.stringify(dict));
};

var handleBattery = function () {
  'use strict';
  if (battery) {
    battery.addEventListener('chargingchange', sendBatteryState);
    battery.addEventListener('levelchange', sendBatteryState);
  }
  sendBatteryState();
};

function GColorFromHex(hex) {
	var hexNum = parseInt(hex, 16);
	var a = 192;
	var r = (((hexNum >> 16) & 0xFF) >> 6) << 4;
	var g = (((hexNum >>  8) & 0xFF) >> 6) << 2;
	var b = (((hexNum >>  0) & 0xFF) >> 6) << 0;
	return a + r + g + b;
}
function GColorToHex(color) {
	var r = (color & 48) >> 4;
	var g = (color & 12) >> 2;
	var b = (color & 3) >> 0;
	var hexString = [r,g,b].map(function (x) {
		x *= 5;
		return x.toString(16) + x.toString(16);
	}).join('');
	return hexString.toUpperCase();
}

Pebble.addEventListener('ready', function (e) {
  'use strict';
  console.log("ready called!");
  initialized = true;
  

	  battery = navigator.battery || navigator.webkitBattery || navigator.mozBattery;
	  console.log("set battery: " + battery);
	  if (navigator.getBattery) {
		console.log("battery API exists.");
		navigator.getBattery().then(function (b) {
		  battery = b;
		  console.log("set battery in then(): " + b);
		  handleBattery();
		});
	  } else {
		handleBattery();
	  }

});


Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
	var localoptions;
	var url;
	if (localStorage.getItem("localOptions") === null) {
		url = 'https://dl.dropboxusercontent.com/s/c9fjw9kb4c5frps/manekineko-setting_test.html?first_start=yes';
	}else {
		localoptions = JSON.parse(localStorage.getItem("localOptions"));
		url = 'https://dl.dropboxusercontent.com/s/c9fjw9kb4c5frps/manekineko-setting_test.html' + '?' +
					'color_text_select=' + localoptions.KEY_TEXT_COLOR + '&' + 
					'color_bkgnd_select=' + localoptions.KEY_BKGND_COLOR + '&' + 
					'show_date_select=' + localoptions.KEY_SHOW_DATE + '&' + 
					'show_battery_select=' + localoptions.KEY_SHOW_BATTERY + '&' + 
					'show_bluetooth_select=' + localoptions.KEY_SHOW_BT + '&' + 
					'show_phone_batt_select=' + localoptions.KEY_SHOW_PHONE_BATT + '&' + 
					'hourly_vibe_select=' + localoptions.KEY_HOURLY_VIBE + '&' + 
					'vibe_bt_select=' + localoptions.KEY_VIBE_BT;
	}
	
	console.log(url);
	Pebble.openURL(url);

});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var options = JSON.parse(decodeURIComponent(e.response));
  options.KEY_PHONE_BATTERY = getBatteryStateInt();
  console.log('Config window returned: ' + options.KEY_TEXT_COLOR);
	
  options.KEY_TEXT_COLOR = GColorFromHex(options.KEY_TEXT_COLOR.substring(2));
  options.KEY_BKGND_COLOR = GColorFromHex(options.KEY_BKGND_COLOR.substring(2));
  
  // Send settings to Pebble watchapp
  Pebble.sendAppMessage(options, function(){
	options.KEY_TEXT_COLOR = '0x' + GColorToHex(options.KEY_TEXT_COLOR);
	options.KEY_BKGND_COLOR = '0x' + GColorToHex(options.KEY_BKGND_COLOR);
	
	console.log('Send successful: ' + JSON.stringify(options));
	localStorage.setItem("localOptions", JSON.stringify(options));
  }, function(e) {
    console.log('Message failed: ' + JSON.stringify(e));
  });
});