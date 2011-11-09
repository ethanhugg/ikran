<?php

$roap = $_POST['roap'];

function sendToRoapProxy($message)
{
  $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);

  $success = socket_connect($socket, "localhost", "7627");

  socket_send($socket, $message . "\n", strlen($message) + 1, 0);

  socket_close($socket);
}

function recvFromRoapProxy()
{
  $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
  
  $success = socket_connect($socket, "localhost", 7628);
  
  $buffer = "";
  $bytesReceived = socket_recv($socket, $buffer, 4096, MSG_WAITALL);
  
  socket_close($socket);
  
  return $buffer;
}
  
if ($roap == Null)
{
  $roap = recvFromRoapProxy();
  echo($roap);
}
else
{
  sendToRoapProxy($roap);
  echo("\nSuccess\n\n" . $roap);
}
?>

