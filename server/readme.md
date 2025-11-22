
# GETでパラメータを渡す方法
curl http://localhost:8080/command?start=1

# POSTでJsonファイルを送る方法
curl -X POST http://localhost:8080/command \
     -H "Content-Type: application/json" \
     --data-binary @config.json

     