var initialized = false;

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

Pebble.addEventListener("ready", function() {
  console.log("ready called!");
  initialized = true;
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
	var localoptions;
	if (localStorage.getItem("localOptions") === null) {
		console.log('https://dl.dropboxusercontent.com/s/7o4ieu94gi7ejp3/manekineko-setting.html?first_start=yes');
		Pebble.openURL('https://dl.dropboxusercontent.com/s/7o4ieu94gi7ejp3/manekineko-setting.html?first_start=yes');
	}else {
		localoptions = JSON.parse(localStorage.getItem("localOptions"));
		console.log('https://dl.dropboxusercontent.com/s/7o4ieu94gi7ejp3/manekineko-setting.html' + '?' +
					'color_text_select=' + localoptions.KEY_TEXT_COLOR + '&' + 
					'color_bkgnd_select=' + localoptions.KEY_BKGND_COLOR + '&' + 
					'show_date_select=' + localoptions.KEY_SHOW_DATE + '&' + 
					'show_battery_select=' + localoptions.KEY_SHOW_BATTERY + '&' + 
					'show_bluetooth_select=' + localoptions.KEY_SHOW_BT + '&' + 
					'vibe_bt_select=' + localoptions.KEY_VIBE_BT);
		Pebble.openURL('https://dl.dropboxusercontent.com/s/7o4ieu94gi7ejp3/manekineko-setting.html' + '?' +
					'color_text_select=' + localoptions.KEY_TEXT_COLOR + '&' + 
					'color_bkgnd_select=' + localoptions.KEY_BKGND_COLOR + '&' + 
					'show_date_select=' + localoptions.KEY_SHOW_DATE + '&' + 
					'show_battery_select=' + localoptions.KEY_SHOW_BATTERY + '&' + 
					'show_bluetooth_select=' + localoptions.KEY_SHOW_BT + '&' + 
					'vibe_bt_select=' + localoptions.KEY_VIBE_BT);
	}

});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var options = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ', JSON.stringify(options));
  
  options.KEY_TEXT_COLOR = GColorFromHex(options.KEY_TEXT_COLOR.substring(2));
  options.KEY_BKGND_COLOR = GColorFromHex(options.KEY_BKGND_COLOR.substring(2));
	
  // Send settings to Pebble watchapp
  Pebble.sendAppMessage(options, function(){
    console.log('Sent config data to Pebble');

	options.KEY_TEXT_COLOR = '0x' + GColorToHex(options.KEY_TEXT_COLOR);
	options.KEY_BKGND_COLOR = '0x' + GColorToHex(options.KEY_BKGND_COLOR);

	localStorage.setItem("localOptions", JSON.stringify(options));
  }, function() {
    console.log('Failed to send config data!');
  });
});