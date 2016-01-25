//#include "cinder/app/AppNative.h"
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

//マウスアクション
#include "cinder/Perlin.h"
#include "Particle.h"
#include "Emitter.h"

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

// global variables //マウスアクション
int			counter = 0;
float		floorLevel = 400.0f;
gl::TextureRef particleImg, emitterImg;
bool		ALLOWFLOOR = false;
bool		ALLOWGRAVITY = false;
bool		ALLOWPERLIN = false;
bool		ALLOWTRAILS = false;
Vec3f		gravity( 0, 0.35f, 0 );
const int	CINDER_FACTOR = 10; // how many times more particles than the Java version

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

extern void renderImage( Vec3f _loc, float _diam, Color _col, float _alpha ){
    gl::pushMatrices();
    gl::translate( _loc.x, _loc.y, _loc.z );
    gl::scale( _diam, _diam, _diam );
    gl::color( _col.r, _col.g, _col.b, _alpha );
    gl::begin( GL_QUADS );
    gl::texCoord(0, 0);    gl::vertex(-.5, -.5);
    gl::texCoord(1, 0);    gl::vertex( .5, -.5);
    gl::texCoord(1, 1);    gl::vertex( .5,  .5);
    gl::texCoord(0, 1);    gl::vertex(-.5,  .5);
    gl::end();
    gl::popMatrices();
}

class LeapApp : public AppBasic {
public:
    Renderer* prepareRenderer() { return new RendererGl( RendererGl::AA_MSAA_2 ); }
    void setup(){
        // ウィンドウの位置とサイズを設定
        setWindowPos( 0, 0 );
        setWindowSize( WindowWidth, WindowHeight );
        
        // 光源を追加する
        //glLightfv( GL_LIGHT0, GL_POSITION, lightPosition);
        glEnable( GL_LIGHT0 );
        //glEnable( GL_LIGHTING );//これをつけると影ができる
        
        // 表示フォントの設定
        mFont = Font( "YuGothic", 32 );//文字の形式、サイズ
        mFontColor = ColorA(0.65, 0.83, 0.58);//文字の色
        mFontColor2 = ColorA(0.83, 0.62, 0.53);//文字の色
        
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

        //フィンガーアクション
        particleImg = gl::Texture::create( loadImage( loadResource( RES_PARTICLE ) ) );
        emitterImg = gl::Texture::create( loadImage( loadResource( RES_EMITTER ) ) );
        
        fingerIsDown = false;
        mFingerPos = getWindowCenter();


        // 描画時に奥行きの考慮を有効にする
        //glEnable(GL_DEPTH_TEST);
        //gl::enableDepthRead();
        //gl::enableDepthWrite();
        
        //アニメーション
        A = 100.0;    //振幅を設定
        w = 1.0;    //角周波数を設定
        p = 0.0;    //初期位相を設定
        t = 0.0;    //経過時間を初期化
        
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
    void keyDown( KeyEvent event ){
        char key = event.getChar();
        
        if( ( key == 'g' ) || ( key == 'G' ) )
            ALLOWGRAVITY = ! ALLOWGRAVITY;
        else if( ( key == 'p' ) || ( key == 'P' ) )
            ALLOWPERLIN = ! ALLOWPERLIN;
        else if( ( key == 't' ) || ( key == 'T' ) )
            ALLOWTRAILS = ! ALLOWTRAILS;
        else if( ( key == 'f' ) || ( key == 'F' ) )
            ALLOWFLOOR = ! ALLOWFLOOR;
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
             fingerIsDown = true;
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
            
            //ジェスチャーが終わった時の状態
            if(gesture.state() == Leap::Gesture::STATE_STOP){
                fingerIsDown = false;
                // 各ジェスチャー固有のパラメーターを取得する
                if ( gesture.type() == Leap::Gesture::Type::TYPE_SWIPE ){//検出したジェスチャーがスワイプ
                    if ((swipeCount == 0 )&&(cirCount < 1 && stapCount < 1 && ktapCount < 1)){
                        swipeCount++;//カウントを増やす
                        winRank = 0;
                        cirCount = -1;
                        stapCount = -1;
                        ktapCount = -1;
                    }
                    if(swipeCount > 0){
                        swipeCount++;
                    }
                    cout << "swipeCount" << swipeCount << "\n" << endl;
                }
                else if ( gesture.type() == Leap::Gesture::Type::TYPE_CIRCLE ){//検出したジェスチャーがサークル
                    if ((cirCount == 0 )&&(swipeCount < 1 && stapCount < 1 && ktapCount < 1)){
                        cirCount++;//カウントを増やす
                        winRank = 0;
                        swipeCount = -1;
                        stapCount = -1;
                        ktapCount = -1;
                    }
                    if(cirCount > 0){
                        cirCount++;
                    }
                    /////コンソールアウト
                    cout << "cirCount" << cirCount << "\n" << endl;
                }
                else if ( gesture.type() == Leap::Gesture::Type::TYPE_KEY_TAP ){//検出したジェスチャーがキータップ
                    if ((ktapCount == 0 )&&(cirCount < 1 && stapCount < 1 && swipeCount < 1)){
                        ktapCount++;//カウントを増やす
                        winRank = 0;
                        swipeCount = -1;
                        stapCount = -1;
                        cirCount = -1;
                    }
                    if(ktapCount > 0){
                        ktapCount++;
                    }
                    /////コンソールアウト
                    cout << "keytapCount" << ktapCount << "\n" << endl;
                }
                else if ( gesture.type() == Leap::Gesture::Type::TYPE_SCREEN_TAP ){//検出したジェスチャーがスクリーンタップ
                    if ((stapCount == 0 )&&(cirCount < 1 && swipeCount < 1 && ktapCount < 1)){
                        stapCount++;//カウントを増やす
                    }
                    if(stapCount > 0){
                        stapCount++;
                        winRank = 0;
                        swipeCount = -1;
                        stapCount = -1;
                        ktapCount = -1;
                    }
                    /////コンソールアウト
                    cout << "stapCount" << stapCount << "\n" << endl;
                }
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
    
        //updateLeapObject();
        //renderFrameParameter();

        //カメラのアップデート処理
        mEye = Vec3f( 0.0f, 0.0f, mCameraDistance );//距離を変える
        //socketCl();//ソケット通信（クライアント側）
        
        //フィンガーアクション
        counter++;
        
        if( fingerIsDown ) {
            if( ALLOWTRAILS && ALLOWFLOOR ) {
                mEmitter.addParticles( 5 * CINDER_FACTOR );
            }
            else {
                mEmitter.addParticles( 10 * CINDER_FACTOR );
            }
        }
        
    }
    
    // Leap Motion関連のセットアップ
    void setupLeapObject(){
        
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
    }
    //描写処理
    void draw(){
        
        socketCl();//ソケット通信（クライアント側）
        gl::clear();
        gl::enableAdditiveBlending();//PNG画像のエッジがなくす
        //"title"描写
        gl::pushMatrices();
        gl::drawString("Client Program", Vec2f(100,50),mFontColor, mFont);
        gl::popMatrices();
        
        gl::pushMatrices();
        drawFingerPosition();//インタラクションボックス
        drawHelp();
        switchWindow();
        gl::popMatrices();
    }
    
    
    //Windowの切り替え
    void switchWindow(){
        
        switch (winRank) {
            case 0:
                //メッセージを選択する画面に遷移
                drawWindow();
                drawHelp();
                break;
                
            case 1:
                //決定したメッセージの質量を決める画面に遷移
                drawWindow1();
                drawHelp();
                break;
            case 2:
                //決定したメッセージの質量を決める画面に遷移
                drawWindow2();
                drawHelp();
                break;
                
            default:
                gl::drawString("入力操作を選んでください", Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
                swipeCount = 0;
                cirCount = 0;
                stapCount = 0;
                ktapCount = 0;
                //"title"描写
                gl::pushMatrices();
                gl::drawString("Client Program", Vec2f(100,50),mFontColor, mFont);
                gl::popMatrices();
                drawHelp();
                break;
        }
    }
    //メッセージを選択するウィンドウ
    void drawWindow(){
        //メッセージ選択をする画面
        gl::clear();
        gl::pushMatrices();
        gl::drawString("Client Program", Vec2f(100,50),mFontColor, mFont);
        gl::drawString("ここはウインドウ０です\nメッセージ選択をする画面です\n", Vec2f(WindowWidth/2,WindowHeight/2+100));
        drawFingerPosition();//インタラクションボックス
        drawTime();
        if(swipeCount > 0){
            drawChoiceMessage();
            drawListArea();//メッセージリストの表示
        }else if (cirCount > 0){
            drawChoiceMessage();
            drawListArea();//メッセージリストの表示
        }else if (ktapCount > 0){
            drawChoiceMessage();
            drawListArea();//メッセージリストの表示
        }else if (stapCount > 0){
            drawChoiceMessage();
            drawListArea();//メッセージリストの表示
        }
        gl::popMatrices();
    }
    
    //メッセージに重みを加えるウィンドウ
    void drawWindow1(){
        //選択したメッセージの質量を決める画面
        gl::clear();
        gl::pushMatrices();
        gl::drawString("Client Program", Vec2f(100,50),mFontColor, mFont);
        gl::drawString("ここはウインドウ１です\n選択したメッセージの質量を決める画面です\n", Vec2f(WindowWidth/2,WindowHeight/2+110));
        drawFingerPosition();//インタラクションボックス
        drawTime();
        if(swipeCount > 0){
            drawMessageCircle();
        }else if (cirCount > 0){
            drawMessageCircle();
        }else if (ktapCount > 0){
            drawMessageCircle();
        }else if (stapCount > 0){
            drawMessageCircle();
        }
        gl::popMatrices();
    }
    
    //終了ウィンドウ
    void drawWindow2(){
        //選択したメッセージの質量を決める画面
        gl::clear();
        gl::drawString("ここはウインドウ2です", Vec2f(WindowWidth/2,WindowHeight/2+110));
        gl::drawString("メッセージを送信しました", Vec2f(WindowWidth/2,WindowHeight/2+140));
        gl::drawString("5秒後に最初の画面に戻ります", Vec2f(WindowWidth/2,WindowHeight/2+160));
        drawFingerPosition();//インタラクションボックス
        drawTime();
        
    }
   
    //メッセージリスト
    void drawListArea(){
        gl::pushMatrices();
        for(int i= 0; i < 9 ; i++){
            if(messageNumber != i){
            gl::drawString(messageList[i],Vec2f(992.5, 145 + ( i * 70 )), mFontColor2, mFont);
            }else{
            gl::drawString(messageList[i],Vec2f(992.5, 145 + ( i * 70 )), mFontColor, mFont);
            }
        }
        gl::popMatrices();
    }
    
    //枠としてのBoxを描く
    void drawBox(){
        gl::color(0.65, 0.83, 0.58);
        gl::drawStrokedRoundedRect(Rectf(0,0,270,50), 5);//角の丸い四角
        setDiffuseColor( ci::ColorA( 0.8, 0.8, 0.8 ) );
    }

    
    //説明を記述する
    void drawHelp(){
        gl::pushMatrices();
        if(winRank == -1){
            gl::drawString("ジェスチャーで操作方法を決定します", Vec2f(200, 700));
            gl::drawString("サークルジェスチャー：Leap Motion上で画面に向かって円を描くジェスチャー", Vec2f(200, 710));
            gl::drawString("スワイプジェスチャー：Leap Motion上で手を仰ぐジェスチャー", Vec2f(200, 720));
            gl::drawString("スクリーンタップジェスチャー：Leap Motion上で画面に向かってタップするジェスチャー", Vec2f(200, 730));
            gl::drawString("キータップジェスチャー：Leap Motion上で地の方向にタップするジェスチャー", Vec2f(200, 740));
        }else if(winRank == 0){
            if(swipeCount > 0){
                gl::drawString("スワイプジェスチャーを選択しました", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("スワイプジェスチャーでメッセージを選択できます", Vec2f(200, 700));
                gl::drawString("ジェスチャーをした時の指の本数によって選択が変化します", Vec2f(200,720));
            }else if (cirCount > 0){
                gl::drawString("サークルジェスチャーを選択しました", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("サークルジェスチャーでメッセージを選択できます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数によって選択が変化します", Vec2f(200,720));
            }else if (ktapCount > 0){
                gl::drawString("キータップジェスチャーを選択しました", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("キータップジェスチャーでメッセージを選択できます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数によって選択が変化します", Vec2f(200,720));
            }else if (stapCount > 0){
                gl::drawString("スクリーンタップジェスチャーを選択しました", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("スクリーンタップジェスチャーでメッセージを選択できます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数によって選択が変化します", Vec2f(200,720));
            }
        }else if(winRank == 1){
            if(swipeCount > 0){
                gl::drawString("スワイプジェスチャーを選択しています", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("スワイプジェスチャーでメッセージに重みをつけます", Vec2f(200, 700));
                gl::drawString("ジェスチャーをした時の指の本数が多いほど増加します", Vec2f(200,720));
            }else if (cirCount > 0){
                gl::drawString("サークルジェスチャーを選択しています", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("サークルジェスチャーでメッセージに重みをつけます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数が多いほど増加します", Vec2f(200,720));
            }else if (ktapCount > 0){
                gl::drawString("キータップジェスチャーを選択しています", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("キータップジェスチャーでメッセージに重みをつけます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数によってが多いほど増加します", Vec2f(200,720));
            }else if (stapCount > 0){
                gl::drawString("スクリーンタップジェスチャーを選択しています", Vec2f(100,100),mFontColor, mFont);
                gl::drawString("スクリーンタップジェスチャーでメッセージに重みをつけます", Vec2f(200,700));
                gl::drawString("ジェスチャーをした時の指の本数によってが多いほど増加します", Vec2f(200,720));
            }
        }
        gl::popMatrices();
    };
    
    void drawTime(){
        //時間経過を計算する関数
        if((winRank >= 0)&&(winRank < 2)){timelimit = 30;}
        else if (winRank == 2){timelimit = 5;}
        
        if (time(&next) != last){
            //時間を１秒進める
            last = next;
            pastSec++;
            timeleft = timelimit - pastSec;
        }else if(((winRank >= 0)&&(winRank < 2))&&(pastSec == 30)){
            //３０秒経過するとリセットする
            pastSec = 0;
            timelimit = 30;//時間をリセットする
            winRank++;
            
        }else if((winRank == 2)&&(pastSec == 5)){
            pastSec = 0;
            timelimit = 5;//時間をリセットする
            winRank++;//戻す
            //リセット
            if(winRank > 2){
                winRank = -1;//戻す
                swipeCount = 0;
                cirCount = 0;
                stapCount = 0;
                ktapCount = 0;
            }
        }
        //描写処理
        gl::pushMatrices();
        gl::drawString("残り時間（秒）："+to_string(timeleft), Vec2f(200,800));//経過時間を表示（１秒間表示を補正）
        gl::popMatrices();
    }

    //枠としてのcircleを描く
    void drawMessageCircle(){
        float sendRadius;//円の半径
        if ((swipeCount > 0 )&&(cirCount < 1 && stapCount < 1 && ktapCount < 1)){
            sendRadius = (A*sin(w*(swipeCount * PI / 180.0) - p) + 200);
        }else if ((cirCount > 0 )&&(stapCount < 1 && swipeCount < 1 && ktapCount < 1)){
            sendRadius = (A*sin(w*(cirCount * PI / 180.0) - p) + 200);
        }else if ((stapCount > 0 )&&(cirCount < 1 && swipeCount < 1 && ktapCount < 1)){
            sendRadius = (A*sin(w*(stapCount * PI / 180.0) - p) + 200);
        }else if ((ktapCount > 0 )&&(cirCount < 1 && swipeCount < 1 && stapCount < 1)){
            sendRadius = (A*sin(w*(ktapCount * PI / 180.0) - p) + 200);
        }
        gl::pushMatrices();
        gl::color(0.65, 0.83, 0.58);
        gl::drawStrokedCircle(Vec2f(545.0, 450.0), sendRadius);
        gl::popMatrices();
        t += speed1;    //時間を進める
        //if(sendRadius > 360.0) sendRadius = 0.0;
        setDiffuseColor( ci::ColorA( 0.8, 0.8, 0.8 ) );
    }

    //メッセージを選択するための関数
    void drawChoiceMessage(){
        
        gl::pushMatrices();
        if((swipeCount == 0)||(cirCount == 0)||(stapCount == 0)||(ktapCount == 0)){
            gl::drawString("ジェスチャーをするとメッセージが選べます",Vec2f(485.0, 450.0), mFontColor, mFont);
        }else if((swipeCount == 1)||(cirCount == 1)||(stapCount == 1)||(ktapCount == 1)){
            messageNumber = 0;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 2)||(cirCount == 2)||(stapCount == 2)||(ktapCount == 2)){
            messageNumber = 1;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 3)||(cirCount == 3)||(stapCount == 3)||(ktapCount == 3)){
            messageNumber = 2;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 4)||(cirCount == 4)||(stapCount == 4)||(ktapCount == 4)){
            messageNumber = 3;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 5)||(cirCount == 5)||(stapCount == 5)||(ktapCount == 5)){
            messageNumber = 4;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 6)||(cirCount == 6)||(stapCount == 6)||(ktapCount == 6)){
            messageNumber = 5;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 7)||(cirCount == 7)||(stapCount == 7)||(ktapCount == 7)){
            messageNumber = 6;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 8)||(cirCount == 8)||(stapCount == 8)||(ktapCount == 8)){
            messageNumber = 7;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if((swipeCount == 9)||(cirCount == 9)||(stapCount == 9)||(ktapCount == 9)){
            messageNumber = 8;
            gl::drawString("あなたのえらんだメッセージは" + messageList[messageNumber] , Vec2f(485.0, 450.0),mFontColor, Font( "YuGothic", 24 ));
        }else if(swipeCount > 9){
            swipeCount = 1;
            messageNumber = -1;
        }else if(cirCount > 9){
            cirCount = 1;
            messageNumber = -1;
        }else if(stapCount > 9){
            stapCount = 1;
            messageNumber = -1;
        }else if(ktapCount > 9){
            ktapCount = 1;
            messageNumber = -1;
        }
    }

    void drawFingerPosition(){
        
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
        
        mFingerPos = Vec2f(index.tipPosition().x,index.tipPosition().y);
        
        //マウスアクション
        mEmitter.exist(Vec2f( x, y ));
        floorLevel = 2 / 3.0f * getWindowHeight();
        gl::popMatrices();
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
        server = gethostbyname("10.70.85.188");//サーバーの作成
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
        std::string tapCountLength = std::to_string(stapCount);//string型に変換
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
        //初期値に戻す
//        cirCount = 0;
//        swipeCount = 0;
//        stapCount = 0;
//        ktapCount = 0;
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
    
    //フォント
    Font mFont;
    Color mFontColor, mFontColor2;
    // Leap Motion
    Leap::Controller mLeap;//ジェスチャーの有効化など...
    Leap::Frame mCurrentFrame;//現在
    Leap::Frame mLastFrame;//最新
    
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
    
    //InteractionBoxの実装
    Leap::InteractionBox iBox;
    
    //メッセージを取得する時に使う
    int messageNumber = -1;
    
    //ソケット通信
    int sockfd, portno = 9999, l;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[256];
    char buffer2[256];
    char buffer3[256];
    int flag = -1;
    
    int cirCount = 0;
    int stapCount = 0;
    int ktapCount = 0;
    int swipeCount = 0;
    int winRank = -1;
    
    
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
    
    int timelimit = 30;
    int timeleft;
    
    //Boxのための変数
    float mLeft = 0.0;//左角のx座標
    float mRight = 1440.0;//右角のx座標
    float mTop = 900.0;//上面のy座標
    float mBottom = 0.0;//下面のy座標
    float mBackSide = 500.0;//前面のz座標
    float mFrontSide = -500.0;//後面のz座標
    
    //マウスアクション
    Emitter		mEmitter;
    bool		fingerIsDown;
    Vec2i		mFingerPos;
    
    float A;  //振幅
    float w;  //角周波数
    float p;  //初期位相
    float t;  //経過時間
    float speed1 = 1.0;    //アニメーションの基準となるスピード
    float speed2 = 1.0;
    float eSize = 0.0;
    
    
};
CINDER_APP_BASIC( LeapApp, RendererGl )

