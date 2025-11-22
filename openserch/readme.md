


https://opensearch.org/downloads/

OpenSearchとOpenSearch Dashboardsをダウンロード. v2系が安定してて良い

##以下macの場合
#javaを入れる
brew install openjdk@17
#リンクを通す
sudo ln -sfn /opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk \
  /Library/Java/JavaVirtualMachines/openjdk-17.jdk
#PATH設定
export JAVA_HOME="/opt/homebrew/Cellar/openjdk@17/17.0.17/libexec/openjdk.jdk/Contents/Home"
export PATH="$JAVA_HOME/bin:$PATH"
source ~/.zshrc
#確認
java -version


#jsolの送信

curl -k -u admin \
  -H "Content-Type: application/json" \
  -XPOST "https://localhost:9200/myindex/_bulk?pretty" \
  --data-binary @bulk.jsonl

bulk形式で最後に改行が必要


