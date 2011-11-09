<?php

$roap = $_POST['roap'];

function sendToRoapProxy($message)
{
  $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);

  $success = socket_connect($socket, "localhost", "7627");

  socket_send($socket, $message . "\n", strlen($message) + 1, 0);

  socket_close($socket);
}

if ($roap == Null)
{
?>

{
  "messageType":"OK",
  "callerSessionId":"1234567890",
  "calleeSessionId":"abcdefghij",
  "seq":1
}

<?php
}
else
{
  sendToRoapProxy($roap);
  echo("\nSuccess\n\n" . $roap);
}
?>

