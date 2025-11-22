# コーディング指針
## クラスにする指針
### 状態を保つ
	•	cv::VideoCapture（カメラの状態）
	•	cv::KalmanFilter（内部状態）
	•	cv::DNN::Net（重み情報）
	•	cv::TrackerCSRT（追跡状態）
	•	cv::StereoBM（パラメータ保持）
### リソースを保持する必要がある
	•	cv::Mat（画像メモリ）
	•	cv::UMat（OpenCL/GPU メモリ）
	•	cv::FileStorage（YAML / XML ハンドル）
	•	cv::VideoWriter（FFmpeg ハンドル）
### 初期化、処理、終了といった段階を踏む必要がある
	•	cv::ORB
	•	cv::SIFT
	•	cv::KalmanFilter
	•	cv::dnn::Net
## クラスにしないルール
### 純粋な画像処理
### パラメータが少ない
### リソースを持たない
## クラスにした場合の指針
### コントラクターは軽く
失敗するような処理は入れない
存在しないと失敗するものをコントラクターに入れる

## 引数のルール
### 1) 引数4個まで → 直接引数
### 2) 5〜7個 → 構造体
### 3) それ以上 → クラス
## 命名規則
### クラス名は大文字
### メソッドは小文字
### メンバにプレフィクスつけない
### 関数名も小文字
### 定数は大文字アンダースコア

#システム構成
#画像処理PC
#＃httpクライアント
libcurl

##httpサーバ
CivetWeb

##ファイル監視ソフト
Fluent Bit

#ログサーバ
##DB
Loki

##ダッシュボード
Grafi

##バックエンド
python
Flask:Webサーバ
watchdog:定期的にファイルを監視