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
#include <iostream>
#include <vector>

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
//std::vector<std::vector<int>> point;//点の座標の入れ物

string messageList[] = {
    
    {"大きな声で"},
    {"頑張れ！"},
    {"もう一度説明して"},
    {"面白い！"},
    {"トイレに行きたい"},
    {"わかった"},
    {"かっこいい！"},
    {"ゆっくり話して"},
    {"わからない"},
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
        mEye			= Vec3f( WindowWidth/2, WindowHeight/2, mCameraDistance );//位置
        mCenter			= Vec3f( WindowWidth/2, WindowHeight/2, 0.0f);//カメラのみる先

        mCam.setEyePoint( mEye );//カメラの位置
        mCam.setCenterOfInterestPoint(mCenter);//カメラのみる先
        //(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
        //fozyはカメラの画角、値が大きいほど透視が強くなり、絵が小さくなる
        //getWindowAspectRatio()はアスペクト比
        //nNearは奥行きの範囲：手前（全方面）
        //zFarは奥行きの範囲：後方（後方面）
        mCam.setPerspective( 45.0f, getWindowAspectRatio(), 300.0f, 3000.0f );//カメラから見える視界の設定
        
        mMayaCam.setCurrentCam(mCam);
        
        //backgroundImageの読み込み
        backgroundImage = gl::Texture(loadImage(loadResource(RES_BACKGROUND_IMAGE)));
        
        //メッセージ画像の読み込み
        ok = gl::Texture(loadImage(loadResource(RES_OK_IMAGE)));
        no = gl::Texture(loadImage(loadResource(RES_NO_IMAGE)));
        again = gl::Texture(loadImage(loadResource(RES_AGAIN_IMAGE)));
        cool = gl::Texture(loadImage(loadResource(RES_HE_IMAGE)));
        fast = gl::Texture(loadImage(loadResource(RES_FAST_IMAGE)));
        fight = gl::Texture(loadImage(loadResource(RES_QUESTION_IMAGE)));
        large = gl::Texture(loadImage(loadResource(RES_LARGE_IMAGE)));
        inter = gl::Texture(loadImage(loadResource(RES_INTER_IMAGE)));
        wc = gl::Texture(loadImage(loadResource(RES_WC_IMAGE)));
        
        //メッセージ画像の読み込み
        ok2 = gl::Texture(loadImage(loadResource(RES_OK2_IMAGE)));
        no2 = gl::Texture(loadImage(loadResource(RES_NO2_IMAGE)));
        again2 = gl::Texture(loadImage(loadResource(RES_AGAIN2_IMAGE)));
        cool2 = gl::Texture(loadImage(loadResource(RES_HE2_IMAGE)));
        fast2 = gl::Texture(loadImage(loadResource(RES_FAST2_IMAGE)));
        fight2 = gl::Texture(loadImage(loadResource(RES_QUESTION2_IMAGE)));
        large2 = gl::Texture(loadImage(loadResource(RES_LARGE2_IMAGE)));
        inter2 = gl::Texture(loadImage(loadResource(RES_INTER2_IMAGE)));
        wc2 = gl::Texture(loadImage(loadResource(RES_WC2_IMAGE)));
        
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
                cirCount = 1;
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_KEY_TAP ){//検出したジェスチャーがキータップ
                Leap::KeyTapGesture keytap( gesture );
            }
            else if ( gesture.type() == Leap::Gesture::Type::TYPE_SCREEN_TAP ){//検出したジェスチャーがスクリーンタップ
                Leap::ScreenTapGesture screentap( gesture );
                tapCount = 1;
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
        
        socketCl();//ソケット通信（クライアント側）
        gl::clear();

        gl::pushMatrices();
        drawInteractionBox3();//インタラクションボックス
        //drawBox();//枠と軸になる線を描写する
        drawListArea();//メッセージリストの表示
        drawImage();
        //drawLeapObject();//手の描写
        gl::popMatrices();
        
    }
    //枠としてのBoxを描く
    void drawBox(){
        gl::pushMatrices();
        // 上面
        gl::drawLine( Vec3f( mLeft, mTop, mBackSide ),Vec3f( mRight, mTop, mBackSide ) );//手前
        gl::drawLine( Vec3f( mRight, mTop, mBackSide ),Vec3f( mRight, mTop, mFrontSide ) );//右
        gl::drawLine( Vec3f( mRight, mTop, mFrontSide ),Vec3f( mLeft, mTop, mFrontSide ) );//奥
        gl::drawLine( Vec3f( mLeft, mTop, mFrontSide ),Vec3f( mLeft, mTop, mBackSide ) );//左
        
        // 中面
        gl::drawLine( Vec3f( mLeft, mTop/2, mBackSide ),Vec3f( mRight, mTop/2, mBackSide ) );//手前
        gl::drawLine( Vec3f( mRight, mTop/2, mBackSide ),Vec3f( mRight, mTop/2, mFrontSide ) );//右
        gl::drawLine( Vec3f( mRight, mTop/2, mFrontSide ),Vec3f( mLeft, mTop/2, mFrontSide ) );//奥
        gl::drawLine( Vec3f( mLeft, mTop/2, mFrontSide ),Vec3f( mLeft, mTop/2, mBackSide ) );//左
        
        
        //中心線
        gl::drawLine( Vec3f( mLeft, mTop/2, 0 ),Vec3f( mRight, mTop/2, 0 ) );//横線
        gl::drawLine( Vec3f( mRight/2, 0, 0 ),Vec3f( mRight/2, mTop, 0 ) );//縦線
        //        gl::drawLine( Vec3f( mRight/2, mTop/2, mBackSide ),Vec3f( mRight/2, mTop/2, mFrontSide ) );//手前から奥
        
        // 下面
        gl::drawLine( Vec3f( mLeft, mBottom, mBackSide ),Vec3f( mRight, mBottom, mBackSide ) );
        gl::drawLine( Vec3f( mRight, mBottom, mBackSide ),Vec3f( mRight, mBottom, mFrontSide ) );
        gl::drawLine( Vec3f( mRight, mBottom, mFrontSide ),Vec3f( mLeft, mBottom, mFrontSide ) );
        gl::drawLine( Vec3f( mLeft, mBottom, mFrontSide ),Vec3f( mLeft, mBottom, mBackSide ) );
        
        // 側面
        gl::drawLine( Vec3f( mLeft, mTop, mFrontSide ),Vec3f( mLeft, mBottom, mFrontSide ) );
        gl::drawLine( Vec3f( mLeft, mTop, mBackSide ),Vec3f( mLeft, mBottom, mBackSide ) );
        gl::drawLine( Vec3f( mRight, mTop, mFrontSide ),Vec3f( mRight, mBottom, mFrontSide ) );
        gl::drawLine( Vec3f( mRight, mTop, mBackSide ),Vec3f( mRight, mBottom, mBackSide ) );
        
        gl::popMatrices();
    }
    //メッセージリスト
    void drawListArea(){
        //"大きな声で"画像
        gl::pushMatrices();
        if( large ) {
            gl::draw( large, Vec2d(97.5, 100));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(97.5, 100));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"頑張れ"画像
        gl::pushMatrices();
        
        if( fight ) {
            gl::draw( fight, Vec2d(97.5, 270));//バックグラウンドイメージを追加
            
        }else{
            gl::drawString("Loading image please wait..",Vec2d(97.5, 270));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"もう一度説明して"画像
        gl::pushMatrices();
        if( again ) {
            gl::draw( again, Vec2d(97.5, 440));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(97.5, 440));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"面白い"画像
        gl::pushMatrices();
        if( inter ) {
            gl::draw( inter, Vec2d(545, 100));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(545, 100));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"トイレにいきたい"画像
        gl::pushMatrices();
        if( wc ) {
            gl::draw( wc, Vec2d(545, 270));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(545, 270));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"わかった"画像
        gl::pushMatrices();
        if( ok ) {
            gl::draw( ok, Vec2d(545, 440));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(545, 440));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"かっこいい"画像
        gl::pushMatrices();
        if( cool ) {
            gl::draw( cool, Vec2d(992.5, 100));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(992.5, 100));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"速い!"画像
        gl::pushMatrices();
        if( fast ) {
            gl::draw( fast, Vec2d(992.5, 270));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(992.5, 270));//ロードする間にコメント
        }
        gl::popMatrices();
        
        //"わからない"画像
        gl::pushMatrices();
        if( no ) {
            gl::draw( no, Vec2d(992.5, 440));//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",Vec2d(992.5, 440));//ロードする間にコメント
        }
        gl::popMatrices();
    }
    //背景画像の描写
    void drawImage(){
        //背景画像
        gl::pushMatrices();
        if( backgroundImage ) {
            gl::draw( backgroundImage, getWindowBounds());//バックグラウンドイメージを追加
        }else{
            gl::drawString("Loading image please wait..",getWindowCenter());//ロードする間にコメント
        }
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
//        //スワイプ
//        mParams =  params::InterfaceGl("GestureParameters", Vec2i(0,200));//パネル作成（引数：パネル名、サイズ）
//        mParams.setPosition(Vec2f(0,200));//場所指定
//        mParams.addParam( "Min Lenagth", &mMinLenagth );//スワイプの長さ
//        mParams.addParam( "Min Velocity", &mMinVelocity );//スワイプの速さ
//        //サークル
//        mParams.addParam( "Min Radius", &mMinRadius );//サークルの半径
//        mParams.addParam( "Min Arc", &mMinArc );//孤の長さ
//        //キータップ
//        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//キータップの速さ
//        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
//        mParams.addParam( "MinDistance", &mMinDistance );//距離
//        //スクリーンタップ
//        mParams.addParam( "MinDownVelocity", &mMinDownVelocity );//スクリーンタップの速さ
//        mParams.addParam( "HistorySeconds", &mHistorySeconds );//秒数
//        mParams.addParam( "MinDistance", &mMinDistance );//距離
        
    }
    
    // Leap Motion関連の描画
    void drawLeapObject(){
        gl::pushMatrices();
        // 手の位置を表示する
        auto frame = mLeap.frame();
        for ( auto hand : frame.hands() ) {
            double red = hand.isLeft() ? 1.0 : 0.0;
            
            setDiffuseColor( ci::ColorA( red, 1.0, 0.5 ) );
            //gl::drawSphere( toVec3f( hand.palmPosition() ), 10 );
            
            gl::drawSphere( toVec3f( hand.palmPosition() ), 10 );
            
            setDiffuseColor( ci::ColorA( red, 0.5, 1.0 ) );
            for ( auto finger : hand.fingers() ){
                const Leap::Finger::Joint jointType[] = {
                    Leap::Finger::Joint::JOINT_MCP,
                    Leap::Finger::Joint::JOINT_PIP,
                    Leap::Finger::Joint::JOINT_DIP,
                    Leap::Finger::Joint::JOINT_TIP,
                };
                
                for ( auto type : jointType ) {
                    gl::drawSphere( toVec3f( finger.jointPosition( type ) ), 10 );
                }
            }
        }
        gl::popMatrices();
        // 元に戻す
        setDiffuseColor( ci::ColorA( 0.8, 0.8, 0.8 ) );
        
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
                messageNumber = -1;
        }
     
        // タッチ状態
        else if( index.touchZone() == Leap::Pointable::Zone::ZONE_TOUCHING ) {
            gl::color(1, 0, 0);
            //１列目
            if (x >= 97.5 && x <= 447.5){
                if (y >= 100 && y <= 250 ) {
                    //大きな声で
                    messageNumber = 0;
                }
                else if (y >= 270 && y <= 420 ) {
                    //頑張れ
                    messageNumber = 1;
                }
                else if (y >= 440 && y <= 590 ) {
                    //もう一度説明して
                    messageNumber = 2;
                }
            }
            //2列目
            else if (x >= 545 && x <= 895 ) {
                if (y >= 100 && y <= 250 ) {
                    //面白い
                    messageNumber = 3;
                }
                else if (y >= 270 && y <= 420 ) {
                    //トイレに行きたい
                    messageNumber = 4;
                }
                else if (y >= 440 && y <= 590 ) {
                    //わかった
                    messageNumber = 5;
                }
            }
            //3列目
            else if (x >= 992.5 && x <= 1342.5 ) {
                if (y >= 100 && y <= 250 ) {
                    messageNumber = 6;
                }
                else if ( y >= 270 && y <= 420 ) {
                    messageNumber = 7;
                }
                else if ( y >= 440 && y <= 590  ) {
                    messageNumber = 8;
                }
            }else{
                messageNumber = -1;
            }
        }
        // タッチ対象外
        else {
            gl::color(0, 0, 1, .05);
            messageNumber = -1;
        }
        gl::drawSolidCircle( Vec2f( x, y ), 10 );//指の位置
        // 指の座標を表示する
        stringstream ss;
        //ss << normalizedPosition.x << ", " << normalizedPosition.y << "\n";//0~1の値で表示
        ss << x << ", " << y << "\n";//ウィンドウサイズの値で表示
        ss << messageNumber << "\n";
        
        auto tbox = TextBox().font( Font( "游ゴシック体", 20 ) ).text ( ss.str() );
        auto texture = gl::Texture( tbox.render() );//テクスチャをつくる
        
        auto textX = (normalizedPosition.x < 0.5) ?
        x : x - texture.getBounds().getWidth();//テキストの位置を計算（x座標）
        auto textY = (normalizedPosition.y > 0.5) ?
        y : y - texture.getBounds().getHeight();//テキストの位置を計算（y座標）
        
        gl::color( 0, 0, 0, 1 );//白色
        gl::translate( textX, textY );//指の隣に移動させる
        gl::draw( texture );//座標を描写
        //drawGestureAction(messageNumber, x, y, textX, textY);//ジェスチャーを使った時の処理を描写

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
    // テクスチャの描画
    void drawTexture(){
        if( mTextTexture ) {
            gl::translate( 0, 100);//位置
            gl::draw( mTextTexture );//描く
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
        
        std::string clientNumber = "0";//0. 一ノ瀬、1. 佐々木、2. 佐藤あ、3. 佐藤ま、4. 菅崎、5. 田中、6.先生
        std::string hansCount = std::to_string(mLastFrame.hands().count());//string型に変換
        std::string cirCountLength = std::to_string(cirCount);//string型に変換
        std::string tapCountLength = std::to_string(tapCount);//string型に変換
        std::string sendMessage = std::to_string(messageNumber);//string型に変換
        std::string dammyMessage = "1";
        
        clientNumber += ',';
        clientNumber += hansCount;
        clientNumber += ',';
        clientNumber += cirCountLength;
        clientNumber += ',';
        clientNumber += tapCountLength;
        clientNumber += ',';
        clientNumber += sendMessage;
        clientNumber += ',';
        clientNumber += dammyMessage;
        
        strcpy(buffer, clientNumber.c_str());
        
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
        cirCount = 0;//初期値に戻す
        tapCount = 0;//初期値に戻す
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
    
    //メッセージテクスチャ
    gl::Texture ok,  no, again, large, cool, fight, fast, inter, wc;
    gl::Texture ok2, no2, again2, large2, cool2, fight2, fast2, inter2, wc2;
    
    
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

