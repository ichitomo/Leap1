#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"


#include "cinder/gl/Texture.h"//画像を描画させたいときに使う
#include "cinder/Text.h"
#include "cinder/MayaCamUI.h"
#include "Leap.h"
#include "cinder/params/Params.h"//パラメーターを動的に扱える
#include "cinder/ImageIo.h"//画像を描画させたいときに使う
#include "cinder/ObjLoader.h"//
#include "cinder/Utilities.h"

//ソケット通信
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace cinder::gl;

struct Messages_base{
    char message[40];
    //int num;
};

Messages_base messageList[] = {
    
    {"わかった"},
    {"わからない"},
    {"もう一度"},
    {"戻って"},
    {"速い"},
    {"大きな声で"},
    {"面白い"},
    {"いいね！"},
    {"すごい！"},
    {"かっこいい"},
    {"なるほど"},
    {"ありがとうございます"},
    {"うわー"},
    {"楽しみ"},
    {"はい"},
    {"いいえ"},
    {"きたー"},
    {"トイレに行きたいです"},
};

void error(const char *msg){
    //エラーメッセージ
    perror(msg);
    exit(0);
}

class LeapApp : public AppNative {
public:

    void setup(){
        // ウィンドウの位置とサイズを設定
        setWindowPos( 0, 0 );
        setWindowSize( WindowWidth, WindowHeight );
        
        // 光源を追加する
        glEnable( GL_LIGHTING );
        glEnable( GL_LIGHT0 );
        
        // 表示フォントの設定
        mFont = Font( "YuGothic", 20 );
        
        // カメラ(視点)の設定
        mCam.setEyePoint( Vec3f( 0.0f, 150.0f, 500.0f ) );//カメラの位置
        mCam.setCenterOfInterestPoint( Vec3f( 0.0f, 0.0f, 1.0f ) );//カメラの中心座標
        mCam.setPerspective( 45.0f, getWindowAspectRatio(), 50.0f, 3000.0f );//カメラから見える視界の設定
        
        mMayaCam.setCurrentCam(mCam);
        
        // アルファブレンディングを有効にする
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl::enable(GL_BLEND);
        
        //backgroundImageの読み込み
        backgroundImage = gl::Texture(loadImage(loadResource("../resources/image.jpg")));
        
        
        // 描画時に奥行きの考慮を有効にする
        gl::enableDepthRead();
        gl::enableDepthWrite();

        
        // Leap Motion関連のセットアップ
        setupLeapObject();
        
    }
    // マウスのクリック
    void mouseDown( MouseEvent event ){
        mMayaCam.mouseDown( event.getPos() );
    }
    
    // マウスのドラッグ
    void mouseDrag( MouseEvent event ){
        mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(),
                           event.isMiddleDown(), event.isRightDown() );
    }
    
    //更新処理
    void update(){
        // フレームの更新
        mLastFrame = mCurrentFrame;
        mCurrentFrame = mLeap.frame();
        
        
        //------ 指定したフレームから今のフレームまでの間に認識したジェスチャーの取得 -----
        //以前のフレームが有効であれば、今回のフレームまでの間に検出したジェスチャーの一覧を取得
        //以前のフレームが有効でなければ、今回のフレームで検出したジェスチャーの一覧を取得する
        auto gestures = mLastFrame.isValid() ? mCurrentFrame.gestures( mLastFrame ) :
        mCurrentFrame.gestures();
        
        
        //----- 検出したジェスチャーを保存（Leap::Gestureクラスのジェスチャー判定） -----
        for(auto gesture : gestures){
            //IDによってジェスチャーを登録する
            auto it = std::find_if(mGestureList.begin(), mGestureList.end(),[gesture](Leap::Gesture g){return g.id() == gesture.id();});//ジェスチャー判定
            auto it_swipe = std::find_if( swipe.begin(), swipe.end(),[gesture]( Leap::Gesture g ){ return g.id() == gesture.id(); } );
            auto it_circle = std::find_if( circle.begin(), circle.end(),[gesture]( Leap::Gesture g ){ return g.id() == gesture.id(); } );
            auto it_keytap = std::find_if( keytap.begin(), keytap.end(),[gesture]( Leap::Gesture g ){ return g.id() == gesture.id(); } );
            auto it_screentap = std::find_if( screentap.begin(), screentap.end(),[gesture]( Leap::Gesture g ){ return g.id() == gesture.id(); } );
            
            //ジェスチャー判定
            if (it != mGestureList.end()) {
                *it = gesture;
            }else{
                mGestureList.push_back( gesture );
            }
            // 各ジェスチャー固有のパラメーターを取得する
            if ( gesture.type() == Leap::Gesture::Type::TYPE_SWIPE ){//検出したジェスチャーがスワイプ
                Leap::SwipeGesture swpie( gesture );
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_CIRCLE ){//検出したジェスチャーがサークル
                Leap::CircleGesture circle( gesture );
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_KEY_TAP ){//検出したジェスチャーがキータップ
                Leap::KeyTapGesture keytap( gesture );
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_SCREEN_TAP ){//検出したジェスチャーがスクリーンタップ
                Leap::ScreenTapGesture screentap( gesture );
            }
            
            //スワイプ
            if ( it_swipe != swipe.end() ) {
                *it_swipe = gesture;
            }
            else {
                swipe.push_back( gesture );
            }
            //サークル
            if ( it_circle != circle.end() ) {
                *it_circle = gesture;
            }
            else {
                circle.push_back( gesture );
            }
            //キータップ
            if ( it_keytap != keytap.end() ) {
                *it_keytap = gesture;
            }
            else {
                keytap.push_back( gesture );
            }
            //スクリーンタップ
            if ( it_screentap != screentap.end() ) {
                *it_screentap = gesture;
            }
            else {
                screentap.push_back( gesture );
            }
        }
        
        // 最後の更新から1秒たったジェスチャーを削除する(タイムスタンプはマイクロ秒単位)
        mGestureList.remove_if( [&]( Leap::Gesture g ){
            return (mCurrentFrame.timestamp() - g.frame().timestamp()) >= (1 * 1000 * 1000); } );
        //スワイプ
        swipe.remove_if( [&]( Leap::Gesture g ){
            return (mCurrentFrame.timestamp() - g.frame().timestamp()) >= (1 * 1000 * 1000); } );
        //サークル
        circle.remove_if( [&]( Leap::Gesture g ){
            return (mCurrentFrame.timestamp() - g.frame().timestamp()) >= (1 * 1000 * 1000); } );
        //キータップ
        keytap.remove_if( [&]( Leap::Gesture g ){
            return (mCurrentFrame.timestamp() - g.frame().timestamp()) >= (1 * 1000 * 1000); } );
        //スクリーンタップ
        screentap.remove_if( [&]( Leap::Gesture g ){
            return (mCurrentFrame.timestamp() - g.frame().timestamp()) >= (1 * 1000 * 1000); } );
        
        //インタラクションボックスの座標のパラメーターの更新
        
        iBox = mCurrentFrame.interactionBox();
        
        mLeft = iBox.center().x - (iBox.width() / 2);
        mRight = iBox.center().x + (iBox.width() / 2);
        mTop = iBox.center().y + (iBox.height() / 2);
        mBaottom = iBox.center().y - (iBox.height() / 2);
        mBackSide = iBox.center().z - (iBox.depth() / 2);
        mFrontSide = iBox.center().z + (iBox.depth() / 2);
        
        
        updateLeapObject();
        renderFrameParameter();
        socketCl();//ソケット通信（クライアント側）
    }
    //描写処理
    void draw(){
        gl::clear();
        //gl::draw(backgroundImage, getWindowBounds());//backgroundImageの描写
        gl::pushMatrices();
        drawLeapObject();//マリオネットの描写
        drawInteractionBox3();//インタラクションボックス
        drawListArea();//メッセージリストの表示
        //gl::draw(backgroundImage, getWindowBounds());//backgroundImageの描写
        
        //----- オブジェファイルの読み込み -----//
//        gl::Texture sTexture;
//        sTexture = gl::Texture(loadImage(loadAsset("texture.jpg")));
//        sTexture.enableAndBind();
//        cinder::TriMesh mySphere;
//        ObjLoader loader( loadFile( "android_arm.obj" ) );
//        loader.load( &mySphere );
//        gl::draw( mySphere );
//        sTexture.unbind();

        gl::popMatrices();
        // パラメーター設定UIを描画する
        //mParams.draw();
        
        
    }
    
    // Leap Motion関連のセットアップ
    void setupLeapObject(){
        
        mRotationMatrix = Leap::Matrix::identity();
        mTotalMotionTranslation = Leap::Vector::zero();
        mTotalMotionScale = 1.0f;
        
        //ジェスチャーを有効にする
        mLeap.enableGesture(Leap::Gesture::Type::TYPE_CIRCLE);//サークル
        mLeap.enableGesture(Leap::Gesture::Type::TYPE_KEY_TAP);//キータップ
        mLeap.enableGesture(Leap::Gesture::Type::TYPE_SCREEN_TAP);//スクリーンタップ
        mLeap.enableGesture(Leap::Gesture::Type::TYPE_SWIPE);//スワイプ
        
        //設定を変える
        //スワイプ
        mMinLenagth = mLeap.config().getFloat( "Gesture.Swipe.MinLength" );
        mMinVelocity = mLeap.config().getFloat( "Gesture.Swipe.MinVelocity" );
        //サークル
        mMinRadius = mLeap.config().getFloat( "Gesture.Circle.MinRadius" );
        mMinArc = mLeap.config().getFloat( "Gesture.Circle.MinArc" );
        //キータップ
        mMinDownVelocity = mLeap.config().getFloat( "Gesture.KeyTap.MinDownVelocity" );
        mHistorySeconds = mLeap.config().getFloat( "Gesture.KeyTap.HistorySeconds" );
        mMinDistance = mLeap.config().getFloat( "Gesture.KeyTap.MinDistance" );
        //スクリーンタップ
        mMinDownVelocity = mLeap.config().getFloat( "Gesture.ScreenTap.MinDownVelocity" );
        mHistorySeconds = mLeap.config().getFloat( "Gesture.ScreenTap.HistorySeconds" );
        mMinDistance = mLeap.config().getFloat( "Gesture.ScreenTap.MinDistance" );
        
        //各ジェスチャーのパラメーター表示内容
        //スワイプ
        mParams =  params::InterfaceGl("GestureParameters", Vec2i(0,200));//パネル作成（引数：パネル名、サイズ）
        mParams.setPosition(Vec2f(0,200));//場所指定
        //mParams.addButton("YES", );
        mParams.addParam( "Min Lenagth", &mMinLenagth );//スワイプの長さ
        mParams.addParam( "Min Velocity", &mMinVelocity );//スワイプの速さ
        //サークル
        //mParams = params::InterfaceGl("CircleParameters", Vec2i(0,100));//パネル作成（引数：パネル名、サイズ）
        mParams.addParam( "Min Radius", &mMinRadius );//サークルの半径
        mParams.addParam( "Min Arc", &mMinArc );//孤の長さ
        //キータップ
        //mParams = params::InterfaceGl("KeytapParameters", Vec2i(0,100));//パネル作成（引数：パネル名、サイズ）
        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//キータップの速さ
        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
        mParams.addParam( "MinDistance", &mMinDistance );//距離
        //スクリーンタップ
        //mParams = params::InterfaceGl("ScreenTapParameters", Vec2i(0,100));//パネル作成（引数：パネル名、サイズ）
        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//スクリーンタップの速さ
        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
        mParams.addParam( "MinDistance", &mMinDistance );//距離
        //mParams.addText("わかった");
    }

    // Leap Motion関連の更新
    void updateLeapObject(){
        
        //------ 指をもってくる -----
        auto thumb = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_THUMB);//親指を持ってくる
        auto index = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_INDEX);//人さし指を持ってくる
        auto middle = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_MIDDLE);//中指を持ってくる
        auto ring = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_RING);//薬指を持ってくる
        auto pinky = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_PINKY);//小指を持ってくる
        
        //------ 移動、回転のパラメーターを変更 -----
        
        //初期値に戻す
        mRotateMatrix0 = 0.0;//親指（向かって右足）の回転
        mRotateMatrix2 = 0.0;//人さし指（向かって右腕）の回転
        mRotateMatrix3 = 0.0;//中指（頭）の回転
        mRotateMatrix4 = 0.0;//薬指（向かって左腕）の回転
        mRotateMatrix5 = 0.0;//小指（向かって左足）の回転
        
        //親指（向かって右足）
        for(auto hand : mCurrentFrame.hands()){
            for (auto finger : thumb) {
                if (finger.hand().isLeft() && finger.isExtended()==false ){//左手の指を曲げていて
                    //向かって右足のrotateのパラメーターを変化させ、回転させる
                    if(finger.hand().sphereRadius() <= 80) {//手にフィットする球の半径が80以下
                        // 前のフレームからの回転量
                        if ( mCurrentFrame.rotationProbability( mLastFrame ) > 0.4 ) {
                            mRotateMatrix0 = finger.hand().sphereRadius()*10;
                        }
                    }
                }
            }
        }
        
        //人差し指（向かって右腕）
        for(auto hand : mCurrentFrame.hands()){
            for (auto finger : index) {
                if (finger.hand().isLeft() && finger.isExtended()==false){//左手の指を曲げていて
                    //向かって右腕のrotateのパラメーターを変化させ、回転させる
                    if(hand.sphereRadius()<=80) {////手にフィットする球の半径が80以下
                        // 前のフレームからの回転量
                        if ( mCurrentFrame.rotationProbability( mLastFrame ) > 0.4 ) {
                            mRotateMatrix2 = hand.sphereRadius()*10;
                        }
                    }
                }else if (finger.hand().isRight() && finger.isExtended()==false){//右手の指を曲げていて
                //喜びの口の角度になるように、口のrotateのパラメーターを変化させ、回転させる
                
                }
            }
        }
        
        
        //中指(頭)
        for(auto hand : mCurrentFrame.hands()){
            for (auto finger : middle) {
                if (finger.hand().isLeft() && finger.isExtended()==false){//左手で曲げていて
                    if(hand.sphereRadius()<=80) {////手にフィットする球の半径が80以下
                        // 前のフレームからの回転量
                        if ( mCurrentFrame.rotationProbability( mLastFrame ) > 0.4 ) {
                            mRotateMatrix3 = hand.sphereRadius()*10;
                        }
                    }
                }
            }
        }
        
        
        //薬指(向かって左腕)
        for(auto hand : mCurrentFrame.hands()){
            for (auto finger : ring) {
                if (finger.hand().isLeft() && finger.isExtended()==false){//左手で曲げていて
                    if(hand.sphereRadius()<=80) {////手にフィットする球の半径が80以下
                        // 前のフレームからの回転量
                        if ( mCurrentFrame.rotationProbability( mLastFrame ) > 0.4 ) {
                            mRotateMatrix4 = hand.sphereRadius()*10;
                        }
                    }
                }
            }
        }
        
        
        //小指(向かって左足)
        for(auto hand : mCurrentFrame.hands()){
            for (auto finger : pinky) {
                if (finger.hand().isLeft() && finger.isExtended()==false){//左手で曲げていて
                    // 前のフレームからの回転量
                    if(hand.sphereRadius()<=80) {//曲げた時
                        if ( mCurrentFrame.rotationProbability( mLastFrame ) > 0.4 ) {
                            mRotateMatrix5 = hand.sphereRadius()*10;
                        }
                    }
                }
            }
        }
        
        //手足の移動と回転
        for(auto hand : mCurrentFrame.hands()){
            if (hand.isLeft()) {
                // 前のフレームからの移動量
                if ( mCurrentFrame.translationProbability( mLastFrame ) > 0.4 ) {
                    // 回転を考慮して移動する
                    mTotalMotionTranslation += mRotationMatrix.rigidInverse().transformDirection( mCurrentFrame.translation( mLastFrame ) );
                }
            }
        }
        
        
        //目の回転（手にフィットするボールの半径で変更させる）
        for(auto hand : mCurrentFrame.hands()){
            if (hand.isRight()) {
                rightEyeAngle = hand.sphereRadius();//右目
                leftEyeAngle = -hand.sphereRadius();//左目
            }
        }
    }
    // フレーム情報の描画
    void renderFrameParameter(){
        //----- TextBoxを使って文字やパラメーターを表示する -----
        
        stringstream ss;//確認用変数
        // 説明
        
        //        ss << "左手で体の操作を行う\n";
        //        ss << "親指で右足の回転を行う\n";
        //        ss << "人さし指で右腕の回転を行う\n";
        //        ss << "薬指で左腕の回転を行う\n";
        //        ss << "小指で左足の回転を行う\n\n";
        //        ss << "右手で顔の表情の操作を行う\n";
        //        ss << "人さし指を曲げて喜び感情\n";
        //        ss << "中指を曲げて怒りの感情\n";
        //        ss << "薬指を曲げて哀しみの表情\n";
        //        ss << "小指を曲げて楽しみの表情\n";
        //        ss << "拡大縮小の追加\n";
        //        ss << "flag\n" << flag;
        
        
        // フレームレート
        ss << "FPS : "<< mCurrentFrame.currentFramesPerSecond() << "\n";
        //検出した指の数
        ss << "指の数 : "<< mCurrentFrame.fingers().count() << "\n";
        //指の座標を取得する
        for(auto finger : mCurrentFrame.fingers()){
            ss << "Finger Position：" << finger.tipPosition().x << ","
            << finger.tipPosition().y << ","
            << finger.tipPosition().z <<"\n";
            
            if (finger.isExtended()) {//指が広げられているといき
                //boneTypeの宣言
                const Leap::Bone::Type boneType[] ={
                    Leap::Bone::Type::TYPE_METACARPAL,
                    Leap::Bone::Type::TYPE_PROXIMAL,
                    Leap::Bone::Type::TYPE_INTERMEDIATE,
                    Leap::Bone::Type::TYPE_DISTAL,
                };
                //骨４種の長さと太さ（指の長さと太さ）のパラメーターを表示
                for (auto type : boneType) {
                    auto bone = finger.bone(type);
                    ss << "指の長さと太さ：" << bone.length() << "," << bone.width() << std::endl;
                }
            }
        }
        
        //インタラクションボックスの座標
        ss << "Width :" << iBox.width() << "mm" << "\n";
        ss << "Height:" << iBox.height() << "mm" << "\n";
        ss << "Depth :" << iBox.depth() << "mm" << "\n";
        ss << "Center:" << iBox.center() << "\n";
        
        
        auto tbox0 = TextBox().alignment( TextBox::LEFT ).font( mFont ).text ( ss.str() ).color(Color( 1.0f, 1.0f, 1.0f )).backgroundColor( ColorA( 0, 0, 0, 0.5f ) );
        mTextTexture0 = gl::Texture( tbox0.render() );
        
        
    }
    // Leap Motion関連の描画
    void drawLeapObject(){
        
        gl::pushMatrices();// 表示座標系の保持
        // カメラ位置を設定する
        gl::setMatrices( mMayaCam.getCamera() );
        drawMarionette();//マリオネット描写
        //色をデフォルトに戻す
        //setDiffuseColor( ci::ColorA( 0.8f, 0.8f, 0.8f, 1.0f ) );
        gl::popMatrices();// 表示座標系を戻す
    }
    
    //インタラクションボックスの作成
    void drawInteractionBox3(){
        
        gl::pushMatrices();
        //gl::draw(backgroundImage, getWindowBounds());//backgroundImageの描写
        
        // 人差し指を取得する
        Leap::Finger index = mLeap.frame().fingers().fingerType( Leap::Finger::Type::TYPE_INDEX )[0];
        if ( !index.isValid() ) {
            return;
        }
        // InteractionBoxの座標に変換する
        Leap::InteractionBox iBox = mLeap.frame().interactionBox();
        Leap::Vector normalizedPosition = iBox.normalizePoint( index.stabilizedTipPosition() );//指の先端の座標(normalizedPositionは0から1の値で表す)
        
        // ウィンドウの座標に変換する
        float x = normalizedPosition.x * WindowWidth;
        float y = WindowHeight - (normalizedPosition.y * WindowHeight);
        
        // ホバー状態
        if ( index.touchZone() == Leap::Pointable::Zone::ZONE_HOVERING ) {
            gl::color(0, 1, 0, 1 - index.touchDistance());
            gl::drawSphere(Vec3f(x,y,1.0f), 10.0);//指の先端
        }
        
        // タッチ状態
        else if( index.touchZone() == Leap::Pointable::Zone::ZONE_TOUCHING ) {
            gl::color(1, 0, 0);
            
            if (x >= 0 && x <= 200){
                if (y >= 40 && y <= 60 ) {
                    messageNumber = 0;
                }
                else if (y >= 80 && y <= 100 ) {
                    messageNumber = 1;
                }
                else if (y >= 120 && y <= 140 ) {
                    messageNumber = 2;
                }
                else if (y >= 160 && y <= 180 ) {
                    messageNumber = 3;
                }
                else if (y >= 200 && y <= 220 ) {
                    messageNumber = 5;
                }
                else if (y >= 240 && y <= 260 ) {
                    messageNumber = 6;
                }
                else if (y >= 280 && y <= 300 ) {
                    messageNumber = 7;
                }
                else if (y >= 320 && y <= 340 ) {
                    messageNumber = 8;
                }
                else if (y >= 360 && y <= 380 ) {
                    messageNumber = 9;
                }
                else if (y >= 400 && y <= 420 ) {
                    messageNumber = 10;
                }
                else if (y >= 440 && y <= 460 ) {
                    messageNumber = 11;
                }
                else if (y >= 480 && y <= 500 ) {
                    messageNumber = 12;
                }
                else if (y >= 520 && y <= 540 ) {
                    messageNumber = 13;
                }
                else if (y >= 560 && y <= 580 ) {
                    messageNumber = 14;
                }
                else if (y >= 600 && y <= 620 ) {
                    messageNumber = 15;
                }
                else if (y >= 640 && y <= 660 ) {
                    messageNumber = 16;
                }
                else if (y >= 680 && y <= 700 ) {
                    messageNumber = 17;
                }
//                else if (y >= 720 && y <= 740 ) {
//                    messageNumber = 18;
//                }
//                else{
//                    messageNumber = -1;
//                }
                
            }
        }
        // タッチ対象外
        else {
            gl::color(0, 0, 1, .05);
        }
        
        gl::drawSolidCircle( Vec2f( x, y ), 10 );//指の位置
        
        // 指の座標を表示する
        stringstream ss;
        //ss << normalizedPosition.x << ", " << normalizedPosition.y << "\n";//0~1の値で表示
        //ss << x << ", " << y << "\n";//ウィンドウサイズの値で表示
        //ss << messageNumber << "\n";
        
        auto tbox = TextBox().font( Font( "游ゴシック体", 20 ) ).text ( ss.str() );
        auto texture = gl::Texture( tbox.render() );//テクスチャをつくる
        
        auto textX = (normalizedPosition.x < 0.5) ?
        x : x - texture.getBounds().getWidth();//テキストの位置を計算（x座標）
        auto textY = (normalizedPosition.y > 0.5) ?
        y : y - texture.getBounds().getHeight();//テキストの位置を計算（y座標）
        
        gl::color( 0, 0, 0, 1 );//白色
        gl::translate( textX, textY );//指の隣に移動させる
        //gl::draw( texture );//座標を描写
        drawGestureAction(messageNumber, x, y, textX, textY);//ジェスチャーを使った時の処理を描写
        gl::popMatrices();
        
        
    }
    //マリオネット
    void drawFace(){
        
        // 顔を描く、顔の色・位置、角度のパラメーターの調整を行う関数
        
        auto index = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_INDEX);
        auto middle = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_MIDDLE);
        auto ring = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_RING);
        auto pinky = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_PINKY);
        
        //人さし指
        for (auto finger : index) {
            if (finger.hand().isRight()){
                if(finger.isExtended()==false) {//曲げた時
                    //嬉
                    //口を描く
                    
                    setDiffuseColor( ci::ColorA( 1.0f, 1.0f, 0 ) );//顔の色をオレンジ色に変更
                    
                    //拡大・縮小のパラメーターの調整
                    /*
                     if ( mCurrentFrame.scaleProbability( mLastFrame ) > 0.4 ) {
                     mTotalMotionScale *= mCurrentFrame.scaleFactor( mLastFrame );
                     mTotalMotionScale2 *= mCurrentFrame.scaleFactor( mLastFrame );
                     if ( mTotalMotionScale < 0.1f ) {
                     mTotalMotionScale = 0.1f;
                     mTotalMotionScale2 = 0.1f;
                     }
                     }
                     *///うざいので一時的に消す
                }else if(finger.isExtended()==true){//広げている時
                    mTotalMotionScale = 1.0f;//大きさを元に戻す
                    mTotalMotionScale2 = 1.0f;//大きさを元に戻す
                }
            }
        }
        
        //中指
        
        for (auto finger : middle) {
            if (finger.hand().isRight()){
                if(finger.isExtended()==false) {//曲げた時
                    
                    //怒
                    //口を描く
                    setDiffuseColor( ci::ColorA( 1.0f, 0, 0 ) );//顔の色を赤に変更
                    
                    //位置、角度のパラメーターの調整
                    rightEyeAngle = -45.0;
                    leftEyeAngle = 45.0;
                    
                    //拡大・縮小のパラメーター調整
                    /*
                     if ( mCurrentFrame.scaleProbability( mLastFrame ) > 0.4 ) {
                     mTotalMotionScale *= mCurrentFrame.scaleFactor( mLastFrame );
                     mTotalMotionScale2 *= mCurrentFrame.scaleFactor( mLastFrame );
                     if ( mTotalMotionScale < 0.1f ) {
                     mTotalMotionScale = 0.1f;
                     mTotalMotionScale2 = 0.1f;
                     }
                     }*///うざいので一時的に消す
                }else if(finger.isExtended()==true){//広げている時
                    mTotalMotionScale = 1.0f;//大きさを元に戻す
                    mTotalMotionScale2 = 1.0f;//大きさを元に戻す
                }
            }
        }
        
        //薬指
        for (auto finger : ring) {
            if (finger.hand().isRight()){//右手の
                if(finger.isExtended()==false) {//曲げた時
                    
                    //哀
                    //口を描く
                    setDiffuseColor( ci::ColorA( 0, 0, 1.0f ) );//顔の色を青に変更
                    
                    //位置、角度のパラメーターの調整
                    rightEyeAngle = 45.0;
                    leftEyeAngle = -45.0;
                    
                    
                    //拡大・縮小のパラメーター調整
                    /*
                     if ( mCurrentFrame.scaleProbability( mLastFrame ) > 0.4 ) {
                     mTotalMotionScale *= mCurrentFrame.scaleFactor( mLastFrame );
                     mTotalMotionScale2 *= mCurrentFrame.scaleFactor( mLastFrame );
                     if ( mTotalMotionScale < 0.1f ) {
                     mTotalMotionScale = 0.1f;
                     mTotalMotionScale2 = 0.1f;
                     }
                     }*///うざいので一時的に消す
                }else if(finger.isExtended()==true){//広げている時
                    mTotalMotionScale = 1.0f;//大きさを元に戻す
                    mTotalMotionScale2 = 1.0f;//大きさを元に戻す
                }
            }
        }
        
        //小指
        for (auto finger : pinky) {
            if (finger.hand().isRight()){//右手の
                if(finger.isExtended()==false) {//曲げた時
                    
                    //楽
                    //口を描く
                    setDiffuseColor( ci::ColorA( 1.0f, 0, 1.0f ) );//顔の色を紫に変更
                    
                    //拡大・縮小のパラメーター調整
                    /*
                     if ( mCurrentFrame.scaleProbability( mLastFrame ) > 0.4 ) {
                     mTotalMotionScale *= mCurrentFrame.scaleFactor( mLastFrame );
                     mTotalMotionScale2 *= mCurrentFrame.scaleFactor( mLastFrame );
                     if ( mTotalMotionScale < 0.1f ) {
                     mTotalMotionScale = 0.1f;
                     mTotalMotionScale2 = 0.1f;
                     }
                     }*///うざいので一時的に消す
                }else if(finger.isExtended()==true){//広げている時
                    mTotalMotionScale = 1.0f;//大きさを元に戻す
                    mTotalMotionScale2 = 1.0f;//大きさを元に戻す
                }
            }
        }
    }
    void drawMarionette(){
        
        //マリオネットを描く関数
        
        //頭
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defFaceTransX,
                     mTotalMotionTranslation.y+defFaceTransY,
                     mTotalMotionTranslation.z+defFaceTransZ );//位置
        glRotatef(-mRotateMatrix3, 1.0f, 0.0f, 0.0f);//回転
        glScalef( mTotalMotionScale, mTotalMotionScale, mTotalMotionScale );//大きさ
        glTranslatef( mTotalMotionTranslation.x/10.0,0.0f,0.0f);//移動
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 80, 100 ) );//実体
        gl::popMatrices();
        
        //右目
        gl::pushMatrices();
        //setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x-defEyeTransX,
                     mTotalMotionTranslation.y+defEyeTransY,
                     mTotalMotionTranslation.z+defEyeTransZ);//位置
        glRotatef(rightEyeAngle, 1.0f, 0.0f, 0.0f);//回転
        glScalef( mTotalMotionScale2/5, mTotalMotionScale2/10, mTotalMotionScale2/10 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //左目
        gl::pushMatrices();
        //setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defEyeTransX,
                     mTotalMotionTranslation.y+defEyeTransY,
                     mTotalMotionTranslation.z+defEyeTransZ);//位置
        glRotatef(leftEyeAngle, 1.0f, 0.0f, 0.0f);//回転
        glScalef( mTotalMotionScale2/5, mTotalMotionScale2/10, mTotalMotionScale2/10 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //元に戻す
        rightEyeAngle = 0.0;//右目の角度
        leftEyeAngle = 0.0;//左目の角度
        defEyeTransX = 20.0;//右目の角度
        defEyeTransY = 20.0;//右目の角度
        defEyeTransZ = 100.0;//左目の角度
        
        //口を描く
        gl::pushMatrices();
        glPointSize(10);
        glLineWidth(10);
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3d(mTotalMotionTranslation.x-20, mTotalMotionTranslation.y + 0, mTotalMotionTranslation.z +defMouseTransZ);
        glVertex3d(mTotalMotionTranslation.x + 0, mTotalMotionTranslation.y-30, mTotalMotionTranslation.z +defMouseTransZ);
        glVertex3d(mTotalMotionTranslation.x + 20, mTotalMotionTranslation.y + 0, mTotalMotionTranslation.z +defMouseTransZ);
        glEnd();
        gl::popMatrices();
        
        //胴体を描く
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defBodyTransX,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
        glScalef( mTotalMotionScale, mTotalMotionScale, mTotalMotionScale );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //右腕を描く
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defBodyTransX+75,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
        glRotatef(mRotateMatrix2, 1.0f, 1.0f, 0.0f);//回転
        glTranslatef( mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
        glScalef( mTotalMotionScale/2, mTotalMotionScale/4, mTotalMotionScale/2 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100,  50, 50 ) );//実体
        
        //二個目
        glTranslatef( defBodyTransX+85,0.0f,0.0f);//移動
        glRotatef(mRotateMatrix2, 1.0f, 1.0f, 0.0f);//回転
        glTranslatef( mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
        // glScalef( mTotalMotionScale/2, mTotalMotionScale/4, mTotalMotionScale/2 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 50,  50, 50 ) );//実体
        gl::popMatrices();
        
        //左腕を描く
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defBodyTransX-75,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
        glRotatef(-mRotateMatrix4, -1.0f, 1.0f, 0.0f);//回転
        glTranslatef( -mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
        glScalef( mTotalMotionScale/2, mTotalMotionScale/4, mTotalMotionScale/2 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 50, 50 ) );//実体
        //二個目
        glTranslatef(defBodyTransX-85,0.0f,0.0f);//移動
        glRotatef(-mRotateMatrix4, -1.0f, 1.0f, 0.0f);//回転
        glTranslatef( -mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 50, 50, 50 ) );//実体
        gl::popMatrices();
        
        //右足を描く
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defBodyTransX+25,
                     mTotalMotionTranslation.y+defBodyTransY-75,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
        glRotatef(mRotateMatrix0, 1.0f, 0.0f, 0.0f);//回転
        glScalef( mTotalMotionScale/4, mTotalMotionScale/2, mTotalMotionScale/2 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //左足を描く
        gl::pushMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
        glTranslatef( mTotalMotionTranslation.x+defBodyTransX-25,
                     mTotalMotionTranslation.y+defBodyTransY-75,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
        glRotatef(mRotateMatrix5, 1.0f, 0.0f, 0.0f);//回転
        glScalef( mTotalMotionScale/4, mTotalMotionScale/2, mTotalMotionScale/2 );//大きさ
        gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
    }
    //ジェスチャー
    void drawGestureAction(int messageNumber, int x, int y, float textX, float textY){
        stringstream aa;
        
        
        //auto index = mCurrentFrame.fingers().fingerType(Leap::Finger::Type::TYPE_INDEX);//人さし指を持ってくる
        //---------------　　各ジェスチャーのアクション　　---------------
        
        //サークル
        for(auto gesture : circle){
            gl::pushMatrices();
            aa << messageList[messageNumber].message << std::endl;//メッセージリストから対象のメッセージをとってくる
            auto mbox = TextBox()
            .font( Font( "游ゴシック体", gesture.radius() * 2 ) )
            .text ( aa.str() )
            .backgroundColor(ColorA(0,0,0));
            
            auto texture = gl::Texture( mbox.render() );
            //gl::translate( textX, textY );//指の隣に移動させる
            gl::draw( texture );//メッセージを表示
            gl::popMatrices();
            
        }
    }
    //メッセージリスト
    void drawListArea(){
        stringstream mm;
        
        //リストを表示する
        mm << "\n";
        mm << "わかった" << "\n";
        mm << "わからない" << "\n";
        mm << "もう一度" << "\n";
        mm << "戻って" << "\n";
        mm << "速い" << "\n";
        mm << "大きな声で" << "\n";
        mm << "面白い" << "\n";
        mm << "いいね！" << "\n";
        mm << "すごい！" << "\n";
        mm << "かっこいい" << "\n";
        mm << "なるほど" << "\n";
        mm << "ありがとうございます" << "\n";
        mm << "うわー" << "\n";
        mm << "楽しみ" << "\n";
        mm << "はい" << "\n";
        mm << "いいえ" << "\n";
        mm << "きたー" << "\n";
        mm << "トイレに行きたいです" << "\n";
        
        //for (int i = 0; i<18; i++) {
        gl::pushMatrices();
        auto tbox0 = TextBox().alignment( TextBox::LEFT ).font( mFont ).text ( mm.str() ).color(Color( 1.0f, 1.0f, 1.0f )).backgroundColor( ColorA( 0, 1.0f, 0, 0 ) );
        auto mTextTexture = gl::Texture( tbox0.render() );
        gl::draw( mTextTexture );
        gl::popMatrices();
        //}
    }
    //骨（手）の形を作る
    void drawBone(Leap::Bone bone){
        
        // InteractionBoxの座標に変換する
        Leap::InteractionBox iBox = mLeap.frame().interactionBox();
        Leap::Vector normalizedPosition = iBox.normalizePoint( bone.prevJoint() );//指の先端の座標
        
        // ウィンドウの座標に変換する
       // float x = normalizedPosition.x * WindowWidth;
       // float y = WindowHeight - (normalizedPosition.y * WindowHeight);
        //手の書き方
        //中心
        //gl::pushMatrices();
        //setDiffuseColor(ci::ColorA(1,0,0));
        //gl::drawSphere(toVec3f(bone.center()), 5);
        //手の平側
        //setDiffuseColor(ci::ColorA(0,1,0));
        //gl::drawColorCube(toVec3f(bone.nextJoint()), Vec3f(7,7,7));
        //指先側
        //setDiffuseColor(ci::ColorA(0,0,1));
        //gl::drawSphere(Vec3f(x,y,-1.0f), 5);
        //gl::popMatrices();
        
        //指先
//        //gl::enableDepthWrite();
//        //gl::clear(Color(0.0f, 0.1f, 0.2f));
//        glDisable(GL_CULL_FACE);
//        gl::pushMatrices();
//        //gl::draw(androidHead);
//        //loader.load(&androidHead);
//        gl::popMatrices();
        
    }
    
    // テクスチャの描画
    void drawTexture(int x, int y){
        
        if( tbox0 ) {
            gl::pushMatrices();
            gl::translate( x, y);//位置
            gl::draw( tbox0 );//描く
            gl::popMatrices();
        }
       
    }
    
    // GL_LIGHT0の色を変える
    void setDiffuseColor( ci::ColorA diffuseColor ){
        gl::color( diffuseColor );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, diffuseColor );
    }
    
    // Leap SDKのVectorをCinderのVec3fに変換する
    Vec3f toVec3f( Leap::Vector vec ){
        return Vec3f( vec.x, vec.y, vec.z );
    }
    
    std::string SwipeDirectionToString( Leap::Vector direction ){
        std::string text = "";
        
        const auto threshold = 0.5f;
        
        // 左右
        if ( direction.x < -threshold ) {
            text += "Left";
        }
        else if ( direction.x > threshold ) {
            text += "Right";
        }
        
        // 上下
        if ( direction.y < -threshold ) {
            text += "Down";
        }
        else if ( direction.y > threshold ) {
            text += "Up";
        }
        
        // 前後
        if ( direction.z < -threshold ) {
            text += "Back";
        }
        else if ( direction.z > threshold ) {
            text += "Front";
        }
        
        return text;
    }
    // ジェスチャー種別を文字列にする
    std::string GestureTypeToString( Leap::Gesture::Type type ){
        if ( type == Leap::Gesture::Type::TYPE_SWIPE ) {
            return "swipe";
        }
        else if ( type == Leap::Gesture::Type::TYPE_CIRCLE ) {
            return "circle";
        }
        else if ( type == Leap::Gesture::Type::TYPE_SCREEN_TAP ) {
            return "screen_tap";
        }
        else if ( type == Leap::Gesture::Type::TYPE_KEY_TAP ) {
            return "key_tap";
        }
        
        return "invalid";
    }
    
    // ジェスチャーの状態を文字列にする
    std::string GestureStateToString( Leap::Gesture::State state ){
        if ( state == Leap::Gesture::State::STATE_START ) {
            return "start";
        }
        else if ( state == Leap::Gesture::State::STATE_UPDATE ) {
            return "update";
        }
        else if ( state == Leap::Gesture::State::STATE_STOP ) {
            return "stop";
        }
        
        return "invalid";
    }

    
    void socketCl(){
        //ソケット通信クライアント側
        portno = 9999;//ポート番号
        sockfd = ::socket(AF_INET, SOCK_STREAM, 0);//ソケットの生成
        if (sockfd < 0)
            error("ERROR opening socket");
        server = gethostbyname("localhost");
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
            error("ERROR connecting");
        printf("Please enter the message: ");
        bzero(buffer,256);
        
        //std::string str = std::to_string(mLastFrame.fingers().count());//string型に変換
        //std::string str2 = std::to_string(mLastFrame.hands().count());//string型に変換
        
        //strcpy(buffer,str.c_str());
        //strcpy(buffer,str2.c_str());
        //strcpy(buffer,str);
        //fgets(buffer,255,stdin);
        n = write(sockfd,messageList[messageNumber].message,strlen(messageList[messageNumber].message));//メッセージの発信
        
        //n = write(sockfd,str.c_str(),strlen(buffer));//データの発信
        //n = write(sockfd,str2.c_str(),strlen(buffer));
        //n = write(sockfd, str, strlen(str));
        if (n < 0)
            error("ERROR writing to socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);//データの受信
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n",buffer);
        close(sockfd);
        
    }
    
    //ウィンドウサイズ
    static const int WindowWidth = 1440;
    static const int WindowHeight = 900;
    
    // カメラ
    CameraPersp  mCam;
    MayaCamUI    mMayaCam;
    
    // パラメータ表示用のテクスチャ（メッセージを表示する）
    gl::Texture mTextTexture0;//パラメーター表示用
    //メッセージリスト表示
    gl::Texture tbox0;//大黒柱
    
    //バックグラウンド
    gl::Texture backgroundImage;
    
    //フォント
    Font mFont;
    
    // Leap Motion
    Leap::Controller mLeap;//ジェスチャーの有効化など...
    Leap::Frame mCurrentFrame;//現在
    Leap::Frame mLastFrame;//最新
    
    Leap::Matrix mRotationMatrix;//回転
    Leap::Vector mTotalMotionTranslation;//移動
    
    
    float mRotateMatrix0;//親指（向かって右足）の回転
    float mRotateMatrix2;//人さし指（向かって右腕）の回転
    float mRotateMatrix3;//中指（頭）の回転
    float mRotateMatrix4;//薬指（向かって左腕）の回転
    float mRotateMatrix5;//小指（向かって左足）の回転
    
    //ジェスチャーのための変数追加
    Leap::Frame lastFrame;//最後
    
    //ジェスチャーを取得するための変数
    std::list<Leap::Gesture> mGestureList;
    
    
    //各ジェスチャーを使用する
    std::list<Leap::SwipeGesture> swipe;
    std::list<Leap::CircleGesture> circle;
    std::list<Leap::KeyTapGesture> keytap;
    std::list<Leap::ScreenTapGesture> screentap;
    
    //パラメーター表示する時に使う
    params::InterfaceGl mParams;
    
    //ジェスチャーをするときに取得できる変数
    //スワイプ
    float mMinLenagth;//長さ
    float mMinVelocity;//速さ
    //サークル
    float mMinRadius;//半径
    float mMinArc;//弧
    
    //キータップ、スクリーンタップ
    float mMinDownVelocity;//速さ
    float mHistorySeconds;//秒数
    float mMinDistance;//距離
    
    //マリオネットのための変数
    float mTotalMotionScale = 1.0f;//拡大縮小（顔）
    float mTotalMotionScale2 = 1.0f;//拡大縮小（表情）
    
    float defFaceTransX = 0.0;//顔のx座標の位置
    float defFaceTransY = 0.0;//顔のy座標の位置
    float defFaceTransZ = 50.0;//顔のz座標の位置
    float rightEyeAngle = 0.0;//右目の角度
    float leftEyeAngle = 0.0;//左目の角度
    float defEyeTransX = 20.0;//右目のx座標の位置
    float defEyeTransY = 20.0;//右目のy座標の位置
    float defEyeTransZ = 0.0;//左目のz座標の位置
    float defBodyTransX = 0.0;//体のx座標の位置
    float defBodyTransY = -100.0;//体のy座標の位置
    float defBodyTransZ = 50.0;//体のz座標の位置
    float defMouseTransZ = 110.0;//口のz座標の位置
    
    //bool flag = false;//テキストの文字表示フラグ
    
    //InteractionBoxの実装
    Leap::InteractionBox iBox;
    
    float mLeft;//左の壁
    float mRight;//右の壁
    float mTop;//雲
    float mBaottom;//底
    float mBackSide;//背面
    float mFrontSide;//正面
    
    //int count = 0;
    //メッセージを取得する時に使う
    int messageNumber = -1;

    //int xPosition,yPosition;
    
    //cinder::ObjLoader androidArm;
    //cinder::ObjLoader androidFoot;
    
    //ObjLoader::Face androidHead;
    //ObjLoader androidArm;
    //ObjLoader androidFoot;
    //TriMesh mMesh;//objファイルの読み込み
    
    //ソケット通信
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[256];
    
};
CINDER_APP_NATIVE( LeapApp, RendererGl )

