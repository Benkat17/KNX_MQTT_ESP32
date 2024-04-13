const char config_page_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configuration Wi-Fi</title>


  <style>
    body {
      font-family: Arial, sans-serif; /* Utilisation de la police Arial ou sans-serif si Arial n'est pas disponible */
      background-color: #f0f0f0; /* Couleur de fond de la page */
      margin: 0; /* Suppression des marges par défaut du corps */
      padding: 0; /* Suppression des rembourrages par défaut du corps */
    }


    h2, h3 {
      color: #333; /* Couleur du texte des titres */
      text-align: center; /* Centrer le texte des titres */
    }


    form {
      max-width: 400px; /* Largeur maximale du formulaire */
      margin: 0 auto; /* Centrage horizontal du formulaire */
      padding: 20px; /* Espacement intérieur du formulaire */
      background-color: #fff; /* Couleur de fond du formulaire */
      border-radius: 8px; /* Bordure arrondie du formulaire */
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); /* Ombre légère autour du formulaire */
    }


    label {
      display: block; /* Affichage en bloc pour les labels */
      margin-bottom: 8px; /* Espacement inférieur entre les labels */
    }


    input[type="text"],
    input[type="password"] {
      width: 100%; /* Largeur de saisie de formulaire à 100% */
      padding: 10px; /* Espacement intérieur des saisies de formulaire */
      margin-bottom: 20px; /* Espacement inférieur entre les saisies de formulaire */
      border-radius: 4px; /* Bordure arrondie des saisies de formulaire */
      border: 1px solid #ccc; /* Bordure des saisies de formulaire */
      box-sizing: border-box; /* Inclusion de la bordure et du rembourrage dans la largeur totale */
    }


    input[type="submit"] {
      width: 100%; /* Largeur du bouton de soumission à 100% */
      padding: 10px; /* Espacement intérieur du bouton de soumission */
      border: none; /* Suppression de la bordure du bouton de soumission */
      border-radius: 4px; /* Bordure arrondie du bouton de soumission */
      background-color: #4CAF50; /* Couleur de fond du bouton de soumission */
      color: #fff; /* Couleur du texte du bouton de soumission */
      cursor: pointer; /* Curseur pointeur pour le bouton de soumission */
    }


    input[type="submit"]:hover {
      background-color: #45a049; /* Couleur de fond au survol du bouton de soumission */
    }
  </style>


</head>
<body>
  <h2>Configuration</h2>
  <h3>Configuration Wi-Fi</h3>
  <form action="/configure" method="post">
    <label for="ssid">SSID</label><br>
    <input type="text" id="ssid" name="ssid"><br>


    <label for="password">Mot de passe</label><br>
    <input type="password" id="password" name="password"><br><br>
   
    <label for="address">Adresses</label><br>
    <input type="text" id="address" name="address" placeholder="Séparez vos adresses par des virgules"><br><br>
   
    <label for="address2">Adresses à lire</label><br>
    <input type="text" id="address2" name="address2" placeholder="Séparez vos adresses par des virgules"><br><br>


    <input type="submit" value="Configurer">
  </form>
</body>
</html>
)=====";
