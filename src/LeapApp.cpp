#include "cinder/app/AppNative.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"


#include "cinder/gl/Texture.h"//画像を描画させたいときに使う
#include "cinder/Text.h"
#include "cinder/MayaCamUI.h"
#include "Leap.h"
#include "cinder/params/Params.h"//パラメーターを動的に扱える
#include "cinder/ImageIo.h"//画像を描画させたいときに使う
#include "cinder/ObjLoader.h"//
#include "time.h"
#include "Resources.h"


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

#define PI 3.141592653589793
#define MAXPOINTS 100//記録できる点の限度
GLint point[MAXPOINTS][2];//点の座標の入れ物
string messageList[] = {
    
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
        
        mCameraDistance = 1500.0f;//カメラの距離（z座標）
        mEye			= Vec3f( getWindowWidth()/2, getWindowHeight()/2, mCameraDistance );//位置
        mCenter			= Vec3f( getWindowWidth()/2, getWindowHeight()/2, 0.0f);//カメラのみる先
        //mUp				= Vec3f::yAxis();//頭の方向を表すベクトル
        mCam.setEyePoint( mEye );//カメラの位置
        mCam.setCenterOfInterestPoint(mCenter);//カメラのみる先
        //(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
        //fozyはカメラの画角、値が大きいほど透視が強くなり、絵が小さくなる
        //getWindowAspectRatio()はアスペクト比
        //nNearは奥行きの範囲：手前（全方面）
        //zFarは奥行きの範囲：後方（後方面）
        mCam.setPerspective( 45.0f, getWindowAspectRatio(), 300.0f, 3000.0f );//カメラから見える視界の設定
        
        mMayaCam.setCurrentCam(mCam);
        // アルファブレンディングを有効にする
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl::enable(GL_BLEND);
        // カメラのパラメータを描写するためのセッティング
//        mParams = params::InterfaceGl( "Camera", Vec2i( 200, 160 ) );
//        mParams.setPosition(Vec2i( 1200, 660 ));
//        mParams.addParam( "Scene Rotation", &mSceneRotation, "opened=1" );
//        mParams.addSeparator();
//        mParams.addParam( "Eye Distance", &mCameraDistance, "min=50.0 max=1500.0 step=50.0 keyIncr=s keyDecr=w" );
        
        
        //backgroundImageの読み込み
        backgroundImage = gl::Texture(loadImage(loadResource(RES_BACKGROUND_IMAGE)));
        m1 = gl::Texture(loadImage(loadResource("../resources/m1.jpg")));
        m2 = gl::Texture(loadImage(loadResource("../resources/m2.jpg")));
        
        // 描画時に奥行きの考慮を有効にする
        gl::enableDepthRead();
        
        // Leap Motion関連のセットアップ
        setupLeapObject();
        
        //球体の拡大縮小とsin関数のセッティング
        A = 100.0;    //振幅を設定
        w = 1.0;    //角周波数を設定
        p = 0.0;    //初期位相を設定
        t = 0.0;    //経過時間を初期化
        t2 = 0.0;    //経過時間を初期化
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
//                cirCount++;
//                std::cout << "cirCount:" <<cirCount << "\n" << std::endl;
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_KEY_TAP ){//検出したジェスチャーがキータップ
                Leap::KeyTapGesture keytap( gesture );
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_SCREEN_TAP ){//検出したジェスチャーがスクリーンタップ
                Leap::ScreenTapGesture screentap( gesture );
//                tapCount++;
//                std::cout << "tapCount:" << tapCount << "\n" << std::endl;
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
                //cirCount++;
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
                //tapCount++;
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
        
        iLeft = iBox.center().x - (iBox.width() / 2);
        iRight = iBox.center().x + (iBox.width() / 2);
        iTop = iBox.center().y + (iBox.height() / 2);
        iBaottom = iBox.center().y - (iBox.height() / 2);
        iBackSide = iBox.center().z - (iBox.depth() / 2);
        iFrontSide = iBox.center().z + (iBox.depth() / 2);
        
        
        //updateLeapObject();
        //renderFrameParameter();

        //カメラのアップデート処理
        mEye = Vec3f( 0.0f, 0.0f, mCameraDistance );//距離を変える
        //socketCl();//ソケット通信（クライアント側）
    }
    //描写処理
    void draw(){
        gl::clear();
        
        gl::pushMatrices();
            gl::setMatrices( mMayaCam.getCamera() );
            drawListArea();//メッセージリストの表示
            //drawMessageUI();//MessageUIの描写
            drawMarionette();//マリオネット描写
            drawLeapObject();//手の描写
            drawInteractionBox3();//インタラクションボックス
            //drawListArea();//メッセージリストの表示
            //drawCircle();//値によって球体を拡大縮小させる描写の追加
            //drawSinGraph();//sin関数を描く
            //drawBarGraph();//検知した手の数を棒グラフとして描写していく
//            drawBone();
            drawBox();//枠と軸になる線を描写する
            drawImage();
        drawTexture();
        
        
        gl::popMatrices();
        
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
        mParams.addParam( "Min Lenagth", &mMinLenagth );//スワイプの長さ
        mParams.addParam( "Min Velocity", &mMinVelocity );//スワイプの速さ
        //サークル
        mParams.addParam( "Min Radius", &mMinRadius );//サークルの半径
        mParams.addParam( "Min Arc", &mMinArc );//孤の長さ
        //キータップ
        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//キータップの速さ
        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
        mParams.addParam( "MinDistance", &mMinDistance );//距離
        //スクリーンタップ
        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//スクリーンタップの速さ
        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
        mParams.addParam( "MinDistance", &mMinDistance );//距離
        
    }
    
    //インタラクションボックスの作成
    void drawInteractionBox3(){
     
     gl::pushMatrices();
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
        gl::popMatrices();
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
    
    void drawImage(){
        gl::pushMatrices();
            gl::draw( m1, Vec2d(50,20));
        gl::popMatrices();
        gl::pushMatrices();
            gl::draw( m2, Vec2d(20,50));
        gl::popMatrices();
    }
    // Leap Motion関連の描画
    void drawLeapObject(){
        
            // 手の位置を表示する
            auto frame = mLeap.frame();
            for ( auto hand : frame.hands() ) {
                double red = hand.isLeft() ? 1.0 : 0.0;
                
                setDiffuseColor( ci::ColorA( red, 1.0, 0.5 ) );
                gl::drawSphere( toVec3f( hand.palmPosition() ), 10 );
    
                setDiffuseColor( ci::ColorA( red, 0.5, 1.0 ) );
                for ( auto finger : hand.fingers() ){
                    const Leap::Bone::Type boneType[] = {
                        Leap::Bone::Type::TYPE_METACARPAL,
                        Leap::Bone::Type::TYPE_PROXIMAL,
                        Leap::Bone::Type::TYPE_INTERMEDIATE,
                        Leap::Bone::Type::TYPE_DISTAL,
                    };
                    
                    for ( auto type : boneType ) {
                        auto bone = finger.bone( type );
                        gl::drawSphere( toVec3f( bone.center() ), 10 );
                    }
                }
            }
            
            // 元に戻す
            setDiffuseColor( ci::ColorA( 0.8, 0.8, 0.8 ) );
        
    }
    //マリオネット
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
        

        //胴体を描く
        gl::pushMatrices();
            glTranslatef( mTotalMotionTranslation.x+defBodyTransX,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
            glScalef( mTotalMotionScale, mTotalMotionScale, mTotalMotionScale );//大きさ
            gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //右腕を描く
        gl::pushMatrices();
            glTranslatef( mTotalMotionTranslation.x+defBodyTransX+75,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
            glRotatef(mRotateMatrix2, 1.0f, 1.0f, 0.0f);//回転
            glTranslatef( mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
            glScalef( mTotalMotionScale/2, mTotalMotionScale/4, mTotalMotionScale/2 );//大きさ
            gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100,  50, 50 ) );//実体
        gl::popMatrices();
        
        //左腕を描く
        gl::pushMatrices();
            glTranslatef( mTotalMotionTranslation.x+defBodyTransX-75,
                     mTotalMotionTranslation.y+defBodyTransY,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
            glRotatef(-mRotateMatrix4, -1.0f, 1.0f, 0.0f);//回転
            glTranslatef( -mTotalMotionTranslation.x/10.0,
                     mTotalMotionTranslation.y/10.0,
                     0.0f);//移動
            glScalef( mTotalMotionScale/2, mTotalMotionScale/4, mTotalMotionScale/2 );//大きさ
            gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 50, 50 ) );//実体
        gl::popMatrices();
        
        //右足を描く
        gl::pushMatrices();
            glTranslatef( mTotalMotionTranslation.x+defBodyTransX+25,
                     mTotalMotionTranslation.y+defBodyTransY-75,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
            glRotatef(mRotateMatrix0, 1.0f, 0.0f, 0.0f);//回転
            glScalef( mTotalMotionScale/4, mTotalMotionScale/2, mTotalMotionScale/2 );//大きさ
            gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        
        //左足を描く
        gl::pushMatrices();
            glTranslatef( mTotalMotionTranslation.x+defBodyTransX-25,
                     mTotalMotionTranslation.y+defBodyTransY-75,
                     mTotalMotionTranslation.z+defBodyTransZ);//移動
            glRotatef(mRotateMatrix5, 1.0f, 0.0f, 0.0f);//回転
            glScalef( mTotalMotionScale/4, mTotalMotionScale/2, mTotalMotionScale/2 );//大きさ
            gl::drawColorCube( Vec3f( 0,0,0 ), Vec3f( 100, 100, 100 ) );//実体
        gl::popMatrices();
        setDiffuseColor( ci::ColorA( 0.7f, 0.7f, 0.7f, 1.0f ) );
    }
    //ジェスチャー
    void drawGestureAction(int messageNumber, int x, int y, float textX, float textY){
        stringstream aa;
        
        //---------------　　各ジェスチャーのアクション　　---------------
        
        //サークル
        for(auto gesture : circle){
            gl::pushMatrices();
                aa << messageList[messageNumber]<< std::endl;//メッセージリストから対象のメッセージをとってくる
                auto mbox = TextBox().font( Font( "游ゴシック体", gesture.radius() * 2 ) ).text ( aa.str() )
                    .backgroundColor(ColorA(0,0,0));
                auto texture = gl::Texture( mbox.render() );
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
        auto tbox0 = TextBox().alignment( TextBox::LEFT ).font( mFont ).text ( mm.str() ).color(Color( 1.0f, 1.0f, 1.0f )).backgroundColor( ColorA( 0, 0, 0, 0.2 ) );
        auto mTextTexture = gl::Texture( tbox0.render() );
        gl::draw( mTextTexture );
    }
    //サークル（手の数によって大きくなる球体の描写）
    void drawCircle(){
        gl::pushMatrices();
            gl::drawSphere(Vec3f( 360, 675, -300 ), cirCount, cirCount );//指の位置
        gl::popMatrices();
            t += speed1;    //時間を進める
            if(t > 360.0) t = 0.0;
    }
    //sinグラフを描く
    void drawSinGraph(){
        
        gl::pushMatrices();
            //サイン波を点で静止画として描画///////////////////////////
            for (t1 = 0.0; t1 < WindowWidth; t1 += speed) {
                y = A*sin(w*(t1 * PI / 180.0) - p);
                drawSolidCircle(Vec2f(t1, y + WindowHeight/2), 1);  //円を描く
            }
        
            //点のアニメーションを描画////////////////////////////////
            y = A*sin(w*(t2 * PI / 180.0) - p);
            drawSolidCircle(Vec2f(t2, y + WindowHeight/2), 10);  //円を描く
            t2 += speed;    //時間を進める
            if (t2 > WindowWidth) t2 = 0.0;    //点が右端まで行ったらになったら原点に戻る
        gl::popMatrices();
        
    }
    //時間ごとに座標を記録する関数
    void graphUpdate(){
        //時間が１秒経つごとに座標を配列に記録していく
        if (time(&next) != last){
            last = next;
            pastSec++;
            printf("%d 秒経過\n", pastSec);
            point[pastSec][0]=pastSec;
            point[pastSec][1]=mCurrentFrame.hands().count();
            pointt.x=pastSec;
            pointt.y=mCurrentFrame.hands().count();
        }
    }
    //棒グラフを描く
    void drawBarGraph(){
        for (int i = 0; i < pastSec; i++) {
            //棒グラフを描写していく
            gl::pushMatrices();
                //gl::setMatrices( mMayaCam.getCamera() );
                glBegin(GL_LINE_STRIP);
                glColor3d(1.0, 0.0, 0.0);
                glLineWidth(10);
                glVertex2d(point[i][0]*10, 0);//x座標
                glVertex2d(point[i][0]*10 , point[i][1]*100);//y座標
                glEnd();
            gl::popMatrices();
        }
    }
    //枠としてのBoxを描く
    void drawBox(){
        gl::pushMatrices();
        // 上面
//        gl::drawLine( Vec3f( mLeft, mTop, mBackSide ),Vec3f( mRight, mTop, mBackSide ) );//手前
//        gl::drawLine( Vec3f( mRight, mTop, mBackSide ),Vec3f( mRight, mTop, mFrontSide ) );//右
//        gl::drawLine( Vec3f( mRight, mTop, mFrontSide ),Vec3f( mLeft, mTop, mFrontSide ) );//奥
//        gl::drawLine( Vec3f( mLeft, mTop, mFrontSide ),Vec3f( mLeft, mTop, mBackSide ) );//左
//
        //        //2/3面
        //        gl::drawLine( Vec3f( mLeft, mTop*2/3, mBackSide ),
        //                     Vec3f( mRight, mTop*2/3, mBackSide ) );
        //
        //        gl::drawLine( Vec3f( mRight, mTop*2/3, mBackSide ),
        //                     Vec3f( mRight, mTop*2/3, mFrontSide ) );
        //
        //        gl::drawLine( Vec3f( mRight, mTop*2/3, mFrontSide ),
        //                     Vec3f( mLeft, mTop*2/3, mFrontSide ) );
        //
        //        gl::drawLine( Vec3f( mLeft, mTop*2/3, mFrontSide ),
        //                     Vec3f( mLeft, mTop*2/3, mBackSide ) );
        //
        
//        // 中面
//        gl::drawLine( Vec3f( mLeft, mTop/2, mBackSide ),Vec3f( mRight, mTop/2, mBackSide ) );//手前
//        gl::drawLine( Vec3f( mRight, mTop/2, mBackSide ),Vec3f( mRight, mTop/2, mFrontSide ) );//右
//        gl::drawLine( Vec3f( mRight, mTop/2, mFrontSide ),Vec3f( mLeft, mTop/2, mFrontSide ) );//奥
//        gl::drawLine( Vec3f( mLeft, mTop/2, mFrontSide ),Vec3f( mLeft, mTop/2, mBackSide ) );//左
        
        
        //中心線
        gl::drawLine( Vec3f( mLeft, mTop/2, 0 ),Vec3f( mRight, mTop/2, 0 ) );//横線
        gl::drawLine( Vec3f( mRight/2, 0, 0 ),Vec3f( mRight/2, mTop, 0 ) );//縦線
//        gl::drawLine( Vec3f( mRight/2, mTop/2, mBackSide ),Vec3f( mRight/2, mTop/2, mFrontSide ) );//手前から奥
        
//        // 下面
//        gl::drawLine( Vec3f( mLeft, mBottom, mBackSide ),Vec3f( mRight, mBottom, mBackSide ) );
//        gl::drawLine( Vec3f( mRight, mBottom, mBackSide ),Vec3f( mRight, mBottom, mFrontSide ) );
//        gl::drawLine( Vec3f( mRight, mBottom, mFrontSide ),Vec3f( mLeft, mBottom, mFrontSide ) );
//        gl::drawLine( Vec3f( mLeft, mBottom, mFrontSide ),Vec3f( mLeft, mBottom, mBackSide ) );
//        
//        // 側面
//        gl::drawLine( Vec3f( mLeft, mTop, mFrontSide ),Vec3f( mLeft, mBottom, mFrontSide ) );
//        gl::drawLine( Vec3f( mLeft, mTop, mBackSide ),Vec3f( mLeft, mBottom, mBackSide ) );
//        gl::drawLine( Vec3f( mRight, mTop, mFrontSide ),Vec3f( mRight, mBottom, mFrontSide ) );
//        gl::drawLine( Vec3f( mRight, mTop, mBackSide ),Vec3f( mRight, mBottom, mBackSide ) );
        
        gl::popMatrices();
    }
    //骨（手）の形を作る
    void drawBone(Leap::Bone bone){
        //手の書き方
        gl::pushMatrices();
            //中心
            gl::drawSphere(toVec3f(bone.center()), 5);
            //手の平側
            gl::drawColorCube(toVec3f(bone.nextJoint()), Vec3f(7,7,7));
            //指先側
            gl::drawSphere(Vec3f(x,y,-1.0f), 5);
        gl::popMatrices();
        
    }
    
    // テクスチャの描画
    void drawTexture(){
        if( mTextTexture ) {
            gl::translate( 0, 100);//位置
            gl::draw( mTextTexture );//描く
        }
        if( imgTexture ) {
            //バックグラウンドイメージを追加
            gl::draw( backgroundImage, getWindowBounds());
        }else{
            //ロードする間にコメント
            gl::drawString("Loading image please wait..",getWindowCenter());
        }
    }
    
    // GL_LIGHT0の色を変える
    void setDiffuseColor( ci::ColorA diffuseColor ){
        gl::color( diffuseColor );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, diffuseColor );
    }
    
    
    void socketCl(){
        //ソケット通信クライアント側
        sockfd = ::socket(AF_INET, SOCK_STREAM, 0);//ソケットの生成
        if (sockfd < 0)//socketが作られていない
            error("ERROR opening socket");
        //server = gethostbyname("mima.c.fun.ac.jp");//サーバーの作成
        server = gethostbyname("10.70.87.215");//サーバーの作成
        if (server == NULL) {
            //サーバーにアクセスできない
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
        //printf("Please enter the message: ");
        bzero(buffer,256);
        
        std::string hansCount = std::to_string(mLastFrame.hands().count());//string型に変換
        std::string cirCountLength = std::to_string(cirCount);//string型に変換
        std::string tapCountLength = std::to_string(tapCount);//string型に変換
        
        hansCount += ',';
        hansCount += cirCountLength;
        hansCount += ',';
        hansCount += tapCountLength;
        hansCount += ',';
        hansCount += messageList[5];
        
        strcpy(buffer,hansCount.c_str());
        
        l = write(sockfd,buffer,strlen(buffer));//データの発信
        
        if (l < 0){
            error("ERROR writing to socket");
        }
        
        bzero(buffer,256);
        
        l = read(sockfd,buffer,255);//データの受信
        
        if (l < 0){
            error("ERROR reading from socket");
        }
        printf("%s\n",buffer);
        
        close(sockfd);
    }
    void ges(){
        // Get gestures
        const Leap::GestureList gestures = mLastFrame.gestures();
        for (int g = 0; g < gestures.count(); ++g) {
            
            Leap::Gesture gesture = gestures[g];
            
            switch (gesture.type()) {
                case Leap::Gesture::TYPE_CIRCLE:
                {
                    Leap::CircleGesture circle = gesture;
                    std::string clockwiseness;
                    
                    if (circle.pointable().direction().angleTo(circle.normal()) <= PI/2) {
                        clockwiseness = "clockwise";
                    } else {
                        clockwiseness = "counterclockwise";
                    }
                    
                    // Calculate angle swept since last frame
                    std::cout << std::string(2, ' ')
                    << "Circle id: " << gesture.id()
                    << ", progress: " << circle.progress()
                    << ", radius: " << circle.radius()
                    <<  ", " << clockwiseness << std::endl;
                    break;
                }
                case Leap::Gesture::TYPE_SWIPE:
                {
                    Leap::SwipeGesture swipe = gesture;
                    std::cout << std::string(2, ' ')
                    << "Swipe id: " << gesture.id()
                    << ", direction: " << swipe.direction()
                    << ", speed: " << swipe.speed() << std::endl;
                    break;
                }
                case Leap::Gesture::TYPE_KEY_TAP:
                {
                    Leap::KeyTapGesture tap = gesture;
                    std::cout << std::string(2, ' ')
                    << "Key Tap id: " << gesture.id()
                    << ", position: " << tap.position()
                    << ", direction: " << tap.direction()<< std::endl;
                    break;
                }
                case Leap::Gesture::TYPE_SCREEN_TAP:
                {
                    Leap::ScreenTapGesture screentap = gesture;
                    std::cout << std::string(2, ' ')
                    << "Screen Tap id: " << gesture.id()
                    << ", position: " << screentap.position()
                    << ", direction: " << screentap.direction()<< std::endl;
                    break;
                }
                default:
                    std::cout << std::string(2, ' ')  << "Unknown gesture type." << std::endl;
                    break;
            }
        }
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
   
    //ウィンドウサイズ
    static const int WindowWidth = 1440;
    static const int WindowHeight = 900;
    
    // カメラ
    CameraPersp  mCam;
    MayaCamUI    mMayaCam;
    
    // パラメータ表示用のテクスチャ（メッセージを表示する）
    gl::Texture mTextTexture;//パラメーター表示用
    //メッセージリスト表示
    gl::Texture tbox0;//大黒柱
    
    //バックグラウンド
    gl::Texture backgroundImage;
    gl::Texture m1;
    gl::Texture m2;
    
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
    
    //ci::Vec3f defFaceTrans(new Point3D(0.0, 120.0, 50.0));
    float defFaceTransX = 1080.0;//顔のx座標の位置
    float defFaceTransY = 675+110.0;//顔のy座標の位置
    float defFaceTransZ = 0.0;//顔のz座標の位置
    
    float defBodyTransX = 1080.0;//体のx座標の位置
    float defBodyTransY = 675.0;//体のy座標の位置
    float defBodyTransZ = 0.0;//体のz座標の位置
    
    float defLeftArmTransX=1080.0+75.0;
    float defRightArmTransX=1080.0-75.0;
    float defArmTransY=675+20.0;
    float defArmTransZ=0.0;
    
    float rightEyeAngle = 0.0;//右目の角度
    float leftEyeAngle = 0.0;//左目の角度
    float defEyeTransX = 20.0;//右目のx座標の位置
    float defEyeTransY = 120.0;//右目のy座標の位置
    float defEyeTransZ = 0.0;//左目のz座標の位置
    
    float defMouseTransZ = 0.0;//口のz座標の位置
    //ci::Vec3f mTotalMotionTranslation;//移動
    
    //InteractionBoxの実装
    Leap::InteractionBox iBox;
    float iLeft;//左の壁
    float iRight;//右の壁
    float iTop;//雲
    float iBaottom;//底
    float iBackSide;//背面
    float iFrontSide;//正面
    
    //メッセージを取得する時に使う
    int messageNumber = -1;
    
    //ソケット通信
    int sockfd, portno = 9999, l;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[256];
    char buffer2[256];
    char buffer3[256];
    int cirCount = 0;
    int tapCount = 0;
    
    //カメラをコントロールする
    gl::Texture		imgTexture;
    float				mCameraDistance;
    Vec3f				mEye, mCenter, mUp;
    
    //球体の拡大縮小
    float x, y;  //x, y座標
    float A;  //振幅
    float w;  //角周波数
    float p;  //初期位相
    float t;  //経過時間
    float speed1 = 1.0;    //アニメーションの基準となるスピード
    float speed2 = 1.0;
    float eSize = 0.0;
    //sinグラフ
    float t1;  //静止画用経過時間（X座標）
    float t2;  //アニメーション用経過時間（X座標）
    float speed = 1.0;    //アニメーションのスピード
    //タイマー
    time_t last = time(0);
    time_t next;
    int pastSec = 0;
    //グラフを描写するための座標
    Vec2i pointt;
    
    //Boxのための変数
    float mLeft = 0.0;//左角のx座標
    float mRight = 1440.0;//右角のx座標
    float mTop = 900.0;//上面のy座標
    float mBottom = 0.0;//下面のy座標
    float mBackSide = 500.0;//前面のz座標
    float mFrontSide = -500.0;//後面のz座標
    
    
};
CINDER_APP_NATIVE( LeapApp, RendererGl )

