# Simple Chat Application
Steps to build

`$ git clone https://github.com/LegendWasTaken/SimpleChatApplication.git SimpleChatApp`
<br>
`$ cd SimpleChatApp`
<br>
`$ cmake -DCMAKE_BUILD_TYPE=Release .`
<br>
`$ cmake --build . --target all`

## What is this?
Simple chat application is an application where you can chat.
<br>
Steps to use
1) Follow the steps to build above
2) Open the application twice
3) On one of the applications, click "server", and the other "client"
4) On the client application, set the server information to <br>
"127.0.0.1" (Localhost ip, different if you're trying to connect to someone) <br>
   50000 - Port is typically this, it'll go up by one if it fails. Check the server information displayed
5) Chat to yourself!