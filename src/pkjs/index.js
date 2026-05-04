var myAPIKey = '30cd6fb35672d26b4eb4559d4a512234';

function fetchWeather(latitude, longitude) {
  var url = 'https://api.openweathermap.org/data/2.5/weather?' +
    'lat=' + latitude + '&lon=' + longitude +
    '&appid=' + myAPIKey + '&units=metric';

  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function () {
    if (req.readyState === 4 && req.status === 200) {
      var response = JSON.parse(req.responseText);
      var temperature = Math.round(response.main.temp);
      var conditionCode = response.weather[0].id;

      Pebble.sendAppMessage({
        'KEY_TEMPERATURE': temperature,
        'KEY_CONDITIONS': conditionCode
      }, function () {
        console.log('Weather sent: ' + temperature + '°C, code=' + conditionCode);
      }, function (e) {
        console.log('Failed to send weather: ' + JSON.stringify(e));
      });
    }
  };
  req.onerror = function () {
    console.log('Weather request failed');
  };
  req.send(null);
}

function getLocationAndFetchWeather() {
  navigator.geolocation.getCurrentPosition(
    function (pos) {
      fetchWeather(pos.coords.latitude, pos.coords.longitude);
    },
    function (err) {
      console.log('Location error: ' + err.message);
    },
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready');
  getLocationAndFetchWeather();
});

Pebble.addEventListener('appmessage', function (e) {
  getLocationAndFetchWeather();
});
