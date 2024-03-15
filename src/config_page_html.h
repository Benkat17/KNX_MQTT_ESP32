const char config_page_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Configuration Wi-Fi</title>
</head>
<body>
  <h2>Configuration Wi-Fi</h2>
  <form action="/configure" method="post">
    <label for="ssid">SSID:</label><br>
    <input type="text" id="ssid" name="ssid"><br>
    <label for="password">Mot de passe:</label><br>
    <input type="password" id="password" name="password"><br><br>
    <input type="submit" value="Configurer">
  </form>
</body>
</html>
)=====";
