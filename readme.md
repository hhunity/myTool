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
