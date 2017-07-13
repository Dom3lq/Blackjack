#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <fstream>

#include "ResourcePath.hpp"

using namespace std;
using namespace sf;

RenderWindow window(VideoMode(800, 600), "Blackjack", sf::Style::Titlebar | sf::Style::Close);
Font font;
Event event;
Clock clo;
fstream highscoresFile;
string s;

enum GameScreen{MENU, GAME};
enum Stage{BET, INSURANCE, DECISION, END};

const int START_CASH = 9900;
GameScreen screen = GameScreen::MENU;
int playerCash = START_CASH;
int topCash = START_CASH;

class Score{
public:
    int points;
    string name;
    Score * next;
};

Score * scoresList;

class Card{
public:
    int points = 0;
    bool isAs = false;
    bool hidden = false;
    Sprite * sprite;
    Texture texture;
};

class Bet{
public:
    Card * cards = new Card[10];
    int cardsSize = 0;
    int cash = 0;
    bool insurance = false;
};

class Dealer{
public:
    Card * cards = new Card[10];
    int cardsSize = 0;
};

class Game{
public:
    Stage stage = Stage::BET;
    Bet bets[5];
    Dealer dealer;
    Card deck[4][13];
    int deciding = 0;
    int insurancing = 0;
};

Game *game;
Game *previousGame;

class Button{
public:
    int getWidth();
    int getHeight();
    bool isClicked(int x, int y);
    bool setScale(float x, float y);
    void draw(void);
    RectangleShape * shape;
    Button(string textu, int posx, int posy);
    Button(Sprite * sprite, int posx, int posy);
    Button(string str, int width, int height, int posx, int posy);
    
private:
    bool textured;
    Texture * texture;
    Sprite * sprite;
    Text * text;
    int x, y;
};

Button::Button(string str, int width, int height, int posx, int posy){
    this->x = posx;
    this->y = posy;
    this->textured = false;
    
    this->text = new Text(str, font, 30);
    this->text->setPosition(posx + 20, posy + 5);
    
    this->shape = new RectangleShape(Vector2f(width, height));
    this->shape->setPosition(posx, posy);
    this->shape->setFillColor(*new Color(0, 0, 255, 150));
}

Button::Button(string textu, int posx, int posy){
    this->x = posx;
    this->y = posy;
    this->textured = true;
    if (this->texture->loadFromFile(resourcePath() + "cardback.jpg")) {
        this->sprite = new Sprite(*this->texture);
    }
}

Button::Button(Sprite * sprite, int posx, int posy){
    this->x = posx;
    this->y = posy;
    this->textured = true;
    this->sprite = sprite;
}


bool Button::isClicked(int x, int y){
    if(this->textured){
        this->sprite->setPosition(this -> x, this -> y);
        return (this->sprite->getGlobalBounds().contains(x, y));
    }   else
        return (this->shape->getGlobalBounds().contains(x, y));
}

void Button::draw(){
    if(this->textured){
        this->sprite->setPosition(this -> x, this -> y);
        window.draw(*this->sprite);
    }
    else {
        window.draw(*this->shape);
        window.draw(*this->text);
    }
}

bool Button::setScale(float x, float y){
    if(this->sprite != NULL){
        this->sprite->setScale(x, y);
        return true;
    } else
        return false;
}

Button startButton("Start", 100, 50, 300, 300);
Button clearButton("Clear", 100, 50, 420, 300);
Button backButton("Back", 100, 50, 25, 25);
Button playButton("Play", 100, 50, 350, 200);
Button exitButton("Exit", 100, 50, 350, 300);
Button insureButton("Insure", 150, 50, 300, 300);
Button dontInsureButton("Stay", 100, 50, 470, 300);
Button saveButton("Save", 100, 50, 250, 300);
Button dontSaveButton("Dont Save", 180, 50, 370, 300);

RectangleShape editNameShape(Vector2f(200, 50));
Text nameText("", font, 30);

Text moneyFlowText("", font, 30);

bool withdrawCash(int a){
    if(playerCash >= a){
        playerCash -= a;
        moneyFlowText.setString("-"+to_string(a)+"$");
        moneyFlowText.setFillColor(Color::Red);
        return true;
    } else
        return false;
    
}

void putCash(int a){
    playerCash += a;
    moneyFlowText.setString("+"+to_string(a)+"$");
    moneyFlowText.setFillColor(Color::Green);
    
    if(playerCash > topCash)
        topCash = playerCash;
}

Card lossCard(){
    int i = rand() % 4;
    int j = rand() % 13;
    
    return game->deck[i][j];
}

int getBetCash(int i){
    return game->bets[i].cash;
}

int getDealerPoints(){
    int a = 0;
    for(int i = 0; i < game->dealer.cardsSize; i++){
        a += game->dealer.cards[i].points;
    }
    return a;
}

int getPreviousDealerPoints(){
    int a = 0;
    for(int i = 0; i < previousGame -> dealer.cardsSize; i++){
        a += previousGame->dealer.cards[i].points;
    }
    return a;
}

bool dealerHasBlackJack(){
    
    return (getDealerPoints() == 21);
}

int getPreviousPlayerPoints(int j){
    int a = 0;
    for(int i = 0; i < previousGame->bets[j].cardsSize; i++){
        if( !previousGame->bets[j].cards[i].isAs )
            a+= previousGame->bets[j].cards[i].points;
    }
    
    for(int i = 0; i < previousGame->bets[j].cardsSize; i++){
        if(previousGame->bets[j].cards[i].isAs ){
            if((a+11) <= 21)
                a += 11;
            else
                a++;
        }
    }
    return a;
}


int getPlayerPoints(int j){
    int a = 0;
    for(int i = 0; i < game->bets[j].cardsSize; i++){
        if( !game->bets[j].cards[i].isAs )
            a+= game->bets[j].cards[i].points;
    }
    
    for(int i = 0; i < game->bets[j].cardsSize; i++){
        if(game->bets[j].cards[i].isAs ){
            if((a+11) <= 21)
                a += 11;
            else
                a++;
        }
    }
    return a;
}

bool playerHasBlackJack(int i){
    return (getPlayerPoints(i) == 21);
}

bool isAnyBet(){
    for(int i = 0; i < 5; i++)
        if(game->bets[i].cash > 0)
            return true;
    
    return false;
}

void initCards(){
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 13; j++){
            
            if (game->deck[i][j].texture.loadFromFile(resourcePath() + to_string(i) + to_string(j+2) + ".jpg")) {
                game->deck[i][j].sprite =  new Sprite(game->deck[i][j].texture);
                game->deck[i][j].sprite->setScale(0.25, 0.25);
                
                if(j < 9){
                    game->deck[i][j].points = (j+2);
                } else {
                    game->deck[i][j].points = 10;
                    
                    if(j == 12){
                        game->deck[i][j].isAs = true;
                        game->deck[i][j].points = 11;
                    }
                    
                }
            }
            
        }
}

void resetGame(){
    previousGame = game;
    game = new Game;
    initCards();
}

void finishGame(){
    for(int i = 0; i < 5; i++){
        if((playerHasBlackJack(i) && dealerHasBlackJack()) || (getDealerPoints() < 21 && getDealerPoints() == getPlayerPoints(i)))
            putCash(getBetCash(i));
        else if (playerHasBlackJack(i))
            putCash(getBetCash(i) + (getBetCash(i) * 3 / 2));
        else if (getPlayerPoints(i) > getDealerPoints() && getPlayerPoints(i) < 21)
            putCash(getBetCash(i)*2);
        else if (getDealerPoints() > 21 && getPlayerPoints(i) <= 21)
            putCash(getBetCash(i)*2);
    }
}

void saveHighscores(){
    highscoresFile.open("highscores.txt", fstream::in | fstream::out | fstream::trunc);
    
    if(highscoresFile.is_open()){
        Score * actualScore = scoresList;
        
        if(scoresList){
            highscoresFile << actualScore->name << " " << actualScore->points << "\n";
            
            while(actualScore->next){
                actualScore = actualScore -> next;
                highscoresFile << actualScore->name << " " << actualScore->points << "\n";
            }
        }
        
        highscoresFile.close();
    }
}

void dealerLossCards(){
    game->dealer.cards[0].hidden = false;
    
    for(int i = 2; i < 10; i++){
        if(getDealerPoints() < 17){
            game->dealer.cards[i] = lossCard();
            game->dealer.cardsSize++;
        }
    }
    
    finishGame();
    resetGame();
}

void selectNextPlayer(){
    for(int i = game->deciding + 1; i < 5; i++){
        if(game->bets[i].cash > 0 && !playerHasBlackJack(i)){
            game->deciding = i;
            goto afterSelectingNextPlayer;
        }
    }
    
    dealerLossCards();
afterSelectingNextPlayer:{}
}

void playerLossCard(){
    game->bets[game->deciding].cards[game->bets[game->deciding].cardsSize] = lossCard();
    game->bets[game->deciding].cardsSize++;
    
    if(getPlayerPoints(game->deciding) > 20)
        selectNextPlayer();
}

Sprite backgroundSprite;
Texture backgroundTexture;
RectangleShape menuBackgroundShape(Vector2f(800, 600));
RectangleShape highscoresBackgroundShape(Vector2f(200, 100));
Text highscoresText("Top players: ", font, 30);

void drawMenu(){
    while (window.pollEvent(event))
    {
        if (event.type == Event::Closed) {
            window.close();
            saveHighscores();
        }
        
        if (event.type == Event::MouseButtonPressed){
            int x = event.mouseButton.x;
            int y = event.mouseButton.y;
            
            
            if (playButton.isClicked(x, y)){
                resetGame();
                screen = GameScreen::GAME;
                playerCash = START_CASH;
                topCash = START_CASH;
            } else if (exitButton.isClicked(x, y)){
                window.close();
                saveHighscores();
            }
        }
    }
    
    window.clear();
    
    window.draw(backgroundSprite);
    window.draw(menuBackgroundShape);
    playButton.draw();
    exitButton.draw();
    
    Score * score = scoresList;
    if(score){
        int l = 1;
        while(score -> next && l <= 5){
            score = score -> next;
            l++;
        }
        
        highscoresBackgroundShape.setSize(Vector2f(310, 50+l*50));
        window.draw(highscoresBackgroundShape);
        highscoresText.setPosition(480, 25);
        highscoresText.setString("Top 5 players:");
        window.draw(highscoresText);
        
        l = 0;

        score = scoresList;
        
        highscoresText.setString(score -> name);
        highscoresText.setPosition(480, 75);
        window.draw(highscoresText);
        
        highscoresText.setString(to_string(score -> points) + "$");
        highscoresText.setPosition(650, 75);
        window.draw(highscoresText);
        
        while(score -> next){
            score = score -> next;
            l++;
            
            highscoresText.setString(score -> name);
            highscoresText.setPosition(480, 75 + l * 50);
            window.draw(highscoresText);
            
            highscoresText.setString(to_string(score -> points));
            highscoresText.setPosition(650, 75 + l * 50);
            window.draw(highscoresText);
        }
    }
    
    window.display();

}

Sprite firstTokenSprite;
Sprite secondTokenSprite;
Sprite thirdTokenSprite;
Sprite fourthTokenSprite;

CircleShape selectedTokenShape(20);

Texture firstTokenTexture;
Texture secondTokenTexture;
Texture thirdTokenTexture;
Texture fourthTokenTexture;

Text selectTokenText("Select token to bet ->", font, 30);
Text playerCashText("Player Cash: 0", font, 30);
int selectedToken = 100;

Text betText("", font, 20);

Text pointsText("", font, 20);

Texture cardBackTexture;
Sprite cardBackSprite;

Text whichPlayerText("Player: ", font, 30);

RectangleShape hitShape(Vector2f(100, 50));
Text hitText("Hit", font, 30);

RectangleShape stayShape(Vector2f(100, 50));
Text stayText("Stay", font, 30);

RectangleShape doubleShape(Vector2f(150, 50));
Text doubleText("Double", font, 30);


void initGame(){
    
    window.setFramerateLimit(60);
    menuBackgroundShape.setPosition(0, 0);
    menuBackgroundShape.setFillColor(*new Color(0, 0, 0, 150));
    
    highscoresBackgroundShape.setFillColor(*new Color(0, 0, 0, 50));
    highscoresBackgroundShape.setPosition(470, 20);
    
    if (cardBackTexture.loadFromFile(resourcePath() + "cardback.jpg")) {
        cardBackSprite = *new Sprite(cardBackTexture);
        cardBackSprite.setScale(0.25, 0.25);
    }

    whichPlayerText.setPosition(350, 350);
    
    hitShape.setPosition(250, 300);
    hitShape.setFillColor(*new Color(0, 0, 255, 150));
    hitText.setPosition(270, 305);
    
    stayShape.setPosition(370, 300);
    stayShape.setFillColor(*new Color(0, 0, 255, 150));
    stayText.setPosition(390, 305);
    
    doubleShape.setPosition(490, 300);
    doubleShape.setFillColor(*new Color(0, 0, 255, 150));
    doubleText.setPosition(510, 305);
    
    selectTokenText.setPosition(10, 550);
    playerCashText.setPosition(550, 550);
    moneyFlowText.setPosition(550, 500);
    
    selectedTokenShape.setFillColor(*new Color(0, 0, 255, 150));
    
    if (firstTokenTexture.loadFromFile(resourcePath() + "100token.png")) {
        firstTokenSprite = Sprite(firstTokenTexture);
        firstTokenSprite.setScale(0.25, 0.25);
    }
    
    if (secondTokenTexture.loadFromFile(resourcePath() + "500token.png")) {
        secondTokenSprite = Sprite(secondTokenTexture);
        secondTokenSprite.setScale(0.25, 0.25);
    }
    
    if (thirdTokenTexture.loadFromFile(resourcePath() + "1000token.png")) {
        thirdTokenSprite = Sprite(thirdTokenTexture);
        thirdTokenSprite.setScale(0.25, 0.25);
    }
    
    if (fourthTokenTexture.loadFromFile(resourcePath() + "5000token.png")) {
        fourthTokenSprite = Sprite(fourthTokenTexture);
        fourthTokenSprite.setScale(0.25, 0.25);
    }
    
    saveButton.shape -> setFillColor(Color::Green);
    dontSaveButton.shape -> setFillColor(Color::Red);
    editNameShape.setPosition(300, 200);
    editNameShape.setFillColor(Color::White);
    nameText.setFillColor(Color::Black);
    nameText.setPosition(320, 205);
}

void drawBets(){
    if(game->bets[0].cash > 0){
        betText.setPosition(70, 400);
        betText.setString("Bet: " + to_string(game->bets[0].cash));
        window.draw(betText);
    }
    
    if(game->bets[1].cash > 0){
        betText.setPosition(210, 470);
        betText.setString("Bet: " + to_string(game->bets[1].cash));
        window.draw(betText);
    }
    
    if(game->bets[2].cash > 0){
        betText.setPosition(370, 500);
        betText.setString("Bet: " + to_string(game->bets[2].cash));
        window.draw(betText);
    }
    
    if(game->bets[3].cash > 0){
        betText.setPosition(540, 480);
        betText.setString("Bet: " + to_string(game->bets[3].cash));
        window.draw(betText);
    }
    
    if(game->bets[4].cash > 0){
        betText.setPosition(685, 410);
        betText.setString("Bet: " + to_string(game->bets[4].cash));
        window.draw(betText);
    }
    
}

void drawTokens(){
    for(int i = 0; i < (playerCash%500)/100; i++){
        firstTokenSprite.setPosition(300-(i*2), 550-(i*5));
        window.draw(firstTokenSprite);
    }
    
    for(int i = 0; i < (playerCash%1000)/500; i++){
        secondTokenSprite.setPosition(350-(i*2), 552-(i*5));
        window.draw(secondTokenSprite);
    }
    
    for(int i = 0; i < (playerCash%5000)/1000; i++){
        thirdTokenSprite.setPosition(400-(i*2), 554-(i*5));
        window.draw(thirdTokenSprite);
    }
    
    for(int i = 0; i < playerCash/5000; i++){
        fourthTokenSprite.setPosition(450-(i*2), 552-(i*5));
        window.draw(fourthTokenSprite);
    }
    
    int betsXpos[5] = {50,200,380,550,710};
    int betsYpos[5] = {325,400,430,410,350};
    
    for(int j = 0; j < 5; j++){
        for(int i = 0; i < (game->bets[j].cash%500)/100; i++){
            firstTokenSprite.setPosition(betsXpos[j]-(i*2),betsYpos[j]-(i*5));
            window.draw(firstTokenSprite);
        }
        
        for(int i = 0; i < (game->bets[j].cash%1000)/500; i++){
            secondTokenSprite.setPosition(betsXpos[j] + 30-(i*2),betsYpos[j]-(i*5));
            window.draw(secondTokenSprite);
        }
        
        for(int i = 0; i < (game->bets[j].cash%5000)/1000; i++){
            thirdTokenSprite.setPosition(betsXpos[j]-(i*2),betsYpos[j] + 30-(i*5));
            window.draw(thirdTokenSprite);
        }
        
        for(int i = 0; i < game->bets[j].cash/5000; i++){
            fourthTokenSprite.setPosition(betsXpos[j] + 30-(i*2),betsYpos[j] + 30-(i*5));
            window.draw(fourthTokenSprite);
        }
    }
    
}


void drawCards(){
    Game * toShow;

    if(previousGame != NULL && game->stage == Stage::BET)
        toShow = previousGame;
    else
        toShow = game;
    
    int dealerCardsSize = toShow->dealer.cardsSize;
    for(int i = 0; i < dealerCardsSize; i++){
        Sprite * wsk = toShow->dealer.cards[i].sprite;
        if(toShow->dealer.cards[i].hidden){
            cardBackSprite.setPosition(350 + i * 10, 100);
            window.draw(cardBackSprite);
        } else {
            wsk->setPosition(350 + i * 10, 100);
            window.draw(*wsk);
        }
    }
    
    for(int j = 0; j < 5; j++){
        for(int i = 0; i < toShow->bets[j].cardsSize; i++){
            
            if(toShow->bets[j].cash > 0){
                Sprite * wsk = toShow->bets[j].cards[i].sprite;
                wsk->setPosition(50 + j * 140 + i * 10, 250);
                window.draw(*wsk);
            }
        }
    }
    
    if(previousGame != NULL && game->stage == Stage::BET){
        if(getPreviousDealerPoints() > 0){
            pointsText.setPosition(350, 150);
            
            if(previousGame->dealer.cards[0].hidden)
                pointsText.setString(to_string(previousGame->dealer.cards[1].points));
            else
                pointsText.setString(to_string(getPreviousDealerPoints()));
            
            window.draw(pointsText);
        }
        
        
        for(int i = 0; i < 5; i++){
            if(getPreviousPlayerPoints(i) > 0){
                pointsText.setPosition(50 + i * 140, 200);
                pointsText.setString(to_string(getPreviousPlayerPoints(i)));
                window.draw(pointsText);
            }
        }

    } else {
        if(getDealerPoints() > 0){
            pointsText.setPosition(350, 150);
            
            if(game->dealer.cards[0].hidden)
                pointsText.setString(to_string(game->dealer.cards[1].points));
            else
                pointsText.setString(to_string(getDealerPoints()));
            
            window.draw(pointsText);
        }
        
        
        for(int i = 0; i < 5; i++){
            if(getPlayerPoints(i) > 0){
                pointsText.setPosition(50 + i * 140, 200);
                pointsText.setString(to_string(getPlayerPoints(i)));
                window.draw(pointsText);
            }
        }
    }
}

void drawInsuranceMenu(){
    insureButton.draw();
    dontInsureButton.draw();
    
    whichPlayerText.setString("Player: " + to_string(game->insurancing + 1));
    window.draw(whichPlayerText);
}

void drawDecisionMenu(){
    whichPlayerText.setString("Player: " + to_string(game->deciding + 1));
    window.draw(whichPlayerText);
    
    window.draw(hitShape);
    window.draw(hitText);
    
    window.draw(stayShape);
    window.draw(stayText);
    
    if(game->bets[game->deciding].cardsSize == 2 && playerCash >= game->bets[game->deciding].cash){
        window.draw(doubleShape);
        window.draw(doubleText);
    }
}

void resetBets(){
    for(int i = 0; i < 5; i++){
        putCash(game->bets[i].cash);
        game->bets[i].cash = 0;
    }
}

void insure(){
    game->bets[game->insurancing].insurance = true;
    game->bets[game->insurancing].cash = (game->bets[game->insurancing].cash/2);
    
    for(int i = (game->insurancing + 1); i < 5; i++){
        if(game->bets[i].cash > 0 && !playerHasBlackJack(i)){
            (game->insurancing = i);
            goto afterInsurancing;
        }
    }
    
    if(dealerHasBlackJack()){
        
        for(int i = 0; i < 5; i++){
            if(game->bets[i].cash > 0){
                if (game->bets[i].insurance || playerHasBlackJack(i)){
                    putCash(game->bets[i].cash * 2);
                }
            }
        }
        
        resetGame();
        
    } else {
        game->stage = Stage::DECISION;
        for(int i = 0; i< 5; i++)
            if(game->bets[i].cash > 0 && !playerHasBlackJack(i)){
                game->deciding = i;
                goto afterSelectingDeciding;
            }
        
    afterSelectingDeciding:{}
        
    }
    
afterInsurancing:{}
}

void dontInsure(){
    for(int i = (game->insurancing + 1); i < 5; i++){
        if(game->bets[i].cash > 0  && !playerHasBlackJack(i)){
            (game->insurancing = i);
            goto afterInsurancing2;
        }
    }
    
    if(dealerHasBlackJack()){
        
        for(int i = 0; i < 5; i++){
            if(game->bets[i].cash > 0){
                if (game->bets[i].insurance || playerHasBlackJack(i)){
                    putCash(game->bets[i].cash * 2);
                }
            }
        }
        
        resetGame();
    } else {
        game->stage = Stage::DECISION;
        for(int i = 0; i< 5; i++)
            if(game->bets[i].cash > 0){
                game->deciding = i;
                goto afterSelectingDeciding1;
            }
        
    afterSelectingDeciding1:{}
    }
    
afterInsurancing2:{}
}

void drawEndMenu(){
    window.draw(menuBackgroundShape);
    saveButton.draw();
    dontSaveButton.draw();
    window.draw(editNameShape);
    window.draw(nameText);
}

void saveScore(string name, int points){
    Score * score = scoresList;
    
    if(score){
        if(score -> points <= points){
            scoresList = new Score();
            scoresList -> next = score;
            score = scoresList;
        } else {
            while(score -> next && score -> next -> points > points )
                score = score -> next;
            
            if(!(score -> next)){
                score -> next = new Score();
            } else {
                Score * pomScore = score -> next;;
                score -> next = new Score();
                score -> next -> next = pomScore;
            }
            score = score -> next;
        }
    } else {
        scoresList = new Score();
        score = scoresList;
    }
    
    score -> name = name;
    score -> points = points;
    
}

void drawGame(){
    
    while (window.pollEvent(event)){
        int x = event.mouseButton.x;
        int y = event.mouseButton.y;
        
        if (event.type == Event::Closed) {
            window.close();
            saveHighscores();
        }
        
        if(game -> stage == Stage::END){
            if (event.type == sf::Event::TextEntered)
            {
                if(event.key.code == 8){
                    if(s.size()!=0){
                        s.pop_back();
                    }
                } else if (event.key.code == 10){
                    if(s.size() == 0)
                        s = "Unknown";
                    
                    saveScore(s, topCash);
                    screen = GameScreen::MENU;
                } else if (((event.text.unicode >= 48 && event.text.unicode <= 57) || (event.text.unicode >=65 && event.text.unicode <= 90) || (event.text.unicode >=97 && event.text.unicode <= 122)) && s.length() < 10) {
                    s.push_back((char)event.text.unicode);
                }
                if(s.size() > 0)
                    nameText.setString(s);
                else
                    nameText.setString("Enter name");
            }
        }
        
        if (event.type == Event::MouseButtonPressed){
            if(backButton.isClicked(x, y)){
                game -> stage = Stage::END;
                s = "";
                nameText.setString("Enter name");
            }
            
            if(game -> stage == Stage::END){
                if(saveButton.isClicked(x, y)){
                    if(s.size() == 0)
                        s = "Unknown";
                    
                    saveScore(s, topCash);
                    screen = GameScreen::MENU;
                } else if (dontSaveButton.isClicked(x, y)){
                    screen = GameScreen::MENU;
                }
            } else if (game->stage == Stage::INSURANCE){
                if(insureButton.isClicked(x, y)){
                    insure();
                } else if (dontInsureButton.isClicked(x, y)){
                    dontInsure();
                }
            } else if(game->stage == Stage::BET){
                
                if (event.type == Event::MouseButtonPressed){
                    if(y > 550 && y < 580){
                        if(x > 300 && x < 350)
                            selectedToken = 100;
                        else if (x > 350 && x < 400)
                            selectedToken = 500;
                        else if (x > 400 && x < 450)
                            selectedToken = 1000;
                        else if (x > 450 && x < 500)
                            selectedToken = 5000;
                } else if(clearButton.isClicked(x, y) && isAnyBet()){
                    resetBets();
                }else if (startButton.isClicked(x, y) && isAnyBet()){
                        
                        game->dealer.cards[0] = lossCard();
                        game->dealer.cards[0].hidden = true;
                        
                        game->dealer.cards[1] = lossCard();
                        game->dealer.cardsSize += 2;
                        
                        for(int i = 0; i < 2; i++){
                            for(int j = 0; j < 5; j++){
                                if(game->bets[j].cash > 0){
                                    game->bets[j].cards[i] = lossCard();
                                    game->bets[j].cardsSize++;
                                }
                            }
                        }
                        
                        if(game->dealer.cards[1].isAs){
                            game->stage = Stage::INSURANCE;
                            for(int i = 0; i< 5; i++)
                                if(game->bets[i].cash > 0  && !playerHasBlackJack(i)){
                                    game->insurancing = i;
                                    goto afterSelectingInsurancing;
                                }
                            
                            resetGame();
                        afterSelectingInsurancing:{}
                            
                        }
                        else{
                            game->stage = Stage::DECISION;
                            for(int i = 0; i< 5; i++)
                                if(game->bets[i].cash > 0){
                                    game->deciding = i;
                                    goto afterSelectingDeciding2;
                                }
                            
                        afterSelectingDeciding2:{}
                        }
                        
                } else if(playerCash >= selectedToken){
                    if(x > 20 && x < 110 && y > 310 && y < 425){
                        withdrawCash(selectedToken);
                        game->bets[0].cash += selectedToken;
                    } else if( x > 180 && x < 270 && y > 380 && y < 485){
                        withdrawCash(selectedToken);
                        game->bets[1].cash += selectedToken;
                    } else if (x > 370 && x < 440 && y > 410 && y < 510){
                        withdrawCash(selectedToken);
                        game->bets[2].cash += selectedToken;
                    } else if(x > 540 && x < 630 && y > 400 && y < 470){
                        withdrawCash(selectedToken);
                        game->bets[3].cash += selectedToken;
                    } else if(x > 700 && x < 790 && y > 340 && y < 410){
                        withdrawCash(selectedToken);
                        game->bets[4].cash += selectedToken;
                    }
                
                }
                
            }
            
            } else if (game->stage == Stage::DECISION){
                if(y > 300 && y < 350){
                    if( x > 250 && x < 350){
                        //hit
                        playerLossCard();
                        
                    } else if ( x > 370 && x < 470){
                        //stay
                        selectNextPlayer();
                        
                    } else if (x > 490 && x < 640 && game->bets[game->deciding].cardsSize == 2 && playerCash >= game->bets[game->deciding].cash){
                        //double
                        withdrawCash(game->bets[game->deciding].cash);
                        game->bets[game->deciding].cash = (game->bets[game->deciding].cash * 2);
                        playerLossCard();
                        
                    }
                }
            }
        }
    }
    
    window.clear();
    
    window.draw(backgroundSprite);
    backButton.draw();

    playerCashText.setString("Cash: " + to_string(playerCash) + "$");
    window.draw(playerCashText);
   
    window.draw(moneyFlowText);
    
    drawCards();
    
    drawBets();
    
    if(game->stage == Stage::BET){
        window.draw(selectTokenText);
        
        if(selectedToken == 100)
            selectedTokenShape.setPosition(300, 550);
        else if (selectedToken == 500)
            selectedTokenShape.setPosition(350, 550);
        else if (selectedToken == 1000)
            selectedTokenShape.setPosition(400, 550);
        else
            selectedTokenShape.setPosition(450, 550);
        
        window.draw(selectedTokenShape);
        if(isAnyBet()){
            startButton.draw();
            clearButton.draw();
        }
        
    } else if (game->stage == Stage::DECISION){
        drawDecisionMenu();
    } else if (game -> stage == Stage::END){
        drawEndMenu();
    } else {
        drawInsuranceMenu();
    }

    
    drawTokens();
    
    window.display();
    
}

void loadHighscores(){
    highscoresFile.open("highscores.txt", fstream::in | fstream::out);
    if(highscoresFile.is_open()){
        string actualName;
        bool first = true;
        Score * actualScore;
        Score * prevScore;
        
        while ( highscoresFile >> actualName ){
            prevScore = actualScore;
            actualScore = new Score();
            actualScore->name=actualName;
            highscoresFile >> actualScore->points;
            
            if(first){
                first = false;
                scoresList = actualScore;
            } else {
                prevScore->next = actualScore;
            }
        }
        highscoresFile.close();
    }
}

int main(int, char const**)
{
    loadHighscores();
    
    if (backgroundTexture.loadFromFile(resourcePath() + "blackjack table.png")) {
        backgroundSprite = Sprite(backgroundTexture);
        backgroundSprite.setScale(0.62, 0.8);
    }

    initGame();
    
    if (!font.loadFromFile(resourcePath() + "sansation.ttf")) {
        return EXIT_FAILURE;
    }

    // Start the game loop
    while (window.isOpen())
    {
        if(screen == GameScreen::MENU)
            drawMenu();
        else
            drawGame();
        
    }
    return EXIT_SUCCESS;
}
