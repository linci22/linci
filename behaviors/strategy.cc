#include "naobehavior.h"
#include "../rvdraw/rvdraw.h"

extern int agentBodyType;


//Real game beaming.a
//Filling params x y angle

void NaoBehavior::beam( double& beamX, double& beamY, double& beamAngle ) {
    beamX = -HALF_FIELD_X + worldModel->getUNum();
    beamY = 0;
    beamAngle = 0;
}


SkillType NaoBehavior::selectSkill() {
VecPosition Pos;
VecPosition Posball = worldModel->getBall();
VecPosition Mid = (Posball + VecPosition(-HALF_FIELD_X, 0, 0)) / 2;
double a = (Posball.getY() - 0) / (Posball.getX() - (-HALF_FIELD_X));
double offset_y = 5 / pow((pow(a, 2) + 1), 0.5);
double offset_x = a * offset_y;
VecPosition Target[6] = {
    Posball + VecPosition(-0.5, 0, 0), // 控球位
    VecPosition(-HALF_FIELD_X, HALF_GOAL_Y * Posball.getY() / HALF_FIELD_Y, 0), // 守门位
    VecPosition(9, 4, 0), // 接应位
    VecPosition(9, -4, 0), // 接应位
    VecPosition(Mid + VecPosition(-offset_x, offset_y, 0)), // 防守位
    VecPosition(Mid + VecPosition(offset_x, -offset_y, 0)) // 防守位
};
//将点位移到球场内的合理对应位置中
for(int i =0;i<6;i++)
{
   if (Target[i].getX() > HALF_FIELD_X) {
    Target[i].setX(HALF_FIELD_X);
} else if (Target[i].getX() < -HALF_FIELD_X) {
    Target[i].setX(-HALF_FIELD_X);
}
if (Target[i].getY() > HALF_FIELD_Y) {
    Target[i].setY(HALF_FIELD_Y);
} else if (Target[i].getY() < -HALF_FIELD_Y) {
    Target[i].setY(-HALF_FIELD_Y);
}
}
vector<vector<double>> dis(6, vector<double>(6, 0));
int BotForTarget[6] = {0};
for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
        int playerNum = WO_TEAMMATE1 + j;
        WorldObject* tem;
        if (worldModel->getUNum() == playerNum) {
            Pos = me;
        } else {
            tem = worldModel->getWorldObject(WO_TEAMMATE1 + j);
            if (tem == NULL) { // 如果当前球员位置无效，则把花费设为最大值
                dis[i][j] = 10000;
                continue;
            }
            Pos = tem->pos;
        }
        bool f = worldModel->getFallenTeammate(playerNum);
        if(f)
        {
            dis[i][j] = Target[i].getDistanceTo(Pos)+1;
        }
        else
        {
            dis[i][j] = Target[i].getDistanceTo(Pos);
        }
        
    }
}
for (int i = 0; i < 6; i++) {
    int robot = min_element(dis[i].begin(), dis[i].begin() + 6) -dis[i].begin();
    BotForTarget[i] = robot;
    for (int j = 0; j < 6; j++) {
        dis[j][robot] = 10000;
    }
}
// 清空上一周期的图形
worldModel->getRVSender()->clear();

// 绘制每个阵型点位
for (int i = 0; i < 6; i++) {
    worldModel->getRVSender()->drawPoint("Target" + std::to_string(i), Target[i].getX(), Target[i].getY(), 10.0f, RVSender::GREEN);
}

// 绘制每个球员的位置
for (int i = 0; i < 6; i++) {
    int playerNum = WO_TEAMMATE1 + i;
    WorldObject* tem;
    if (worldModel->getUNum() == playerNum) {
        // 绘制自己的位置为蓝色
        worldModel->getRVSender()->drawPoint("Bot" + std::to_string(i), me.getX(), me.getY(), 10.0f, RVSender::BLUE);
    } else {
        tem = worldModel->getWorldObject(WO_TEAMMATE1 + i);
        if (tem != NULL) {
            // 绘制队员的位置为红色
            worldModel->getRVSender()->drawPoint("Bot" + std::to_string(i), tem->pos.getX(), tem->pos.getY(), 10.0f, RVSender::RED);
        }
    }
}

for (int i = 0; i < 6; i++) {
    if (worldModel->getUNum() == WO_TEAMMATE1 + BotForTarget[i]) 
    {
    if (i == 0) 
    { // 如果我是离球最近的，那么我就跑去控球
    if (me.getDistanceTo(Target[0]) > 1) 
    {
    return goToTarget(collisionAvoidance(true, false, false, 1, .5, Target[0], true));
    } 
    else 
    {
    // 若球在接应点附近且我在控球，就将球踢向距离我较远的对方门柱
    if (Posball.getDistanceTo(Target[2]) <= 2) 
    {
    return kickBall(KICK_FORWARD, VecPosition(HALF_FIELD_X, -HALF_GOAL_Y, 0));
    } 
    if (Posball.getDistanceTo(Target[3]) <= 2) 
    {
    return kickBall(KICK_FORWARD, VecPosition(HALF_FIELD_X, HALF_GOAL_Y, 0));
    }
    if (Posball.getX() < -13.2) 
    { // 若球不在接应点附近且我在控球且我在己方禁区内
    int nearestTeammate = WO_TEAMMATE1; // 初始化
    double minDistanceToMe = 9999; // 初始化
    for (int j = WO_TEAMMATE1; j <= WO_TEAMMATE6; j++) {
    if (worldModel->getUNum() == j) { // 跳过自己
    continue;
    }
    WorldObject* tem = worldModel->getWorldObject(j);
    VecPosition teammatePos = tem->pos;
    double distanceToMe = teammatePos.getDistanceTo(me);
    if (distanceToMe < minDistanceToMe) {
    minDistanceToMe = distanceToMe;
    nearestTeammate = j;
    }
    }
    VecPosition passTarget = Target[0]; // 默认传给控球位置
    for (int j = 0; j < 6; j++) {
    if (BotForTarget[j] == nearestTeammate - WO_TEAMMATE1) {
    passTarget = Target[j];
    break;
    }
    }
    return kickBall(KICK_FORWARD, passTarget);
    } 
    else// 若球不在接应点附近且我在控球且我不在己方禁区内
    { 
    VecPosition passTarget;
    if (me.getDistanceTo(Target[2]) <= me.getDistanceTo(Target[3])) 
    {
    passTarget = Target[2];
    return kickBall(KICK_FORWARD, passTarget);
    } 
    else 
    {
    passTarget = Target[3];
    return kickBall(KICK_FORWARD, passTarget);
    }
    }
    }
    }
    else 
    { // 如果我不是离球最近的，那么我就跑去自己的点位
    return goToTarget(collisionAvoidance(true, false, false, 1, 0.5, Target[i], true));
    }
    }
}
    // My position and angle
    //cout << worldModel->getUNum() << ": " << worldModel->getMyPosition() << ",\t" << worldModel->getMyAngDeg() << "\n";

    // Position of the ball
    //cout << worldModel->getBall() << "\n";

    // Example usage of the roboviz drawing system and RVSender in rvdraw.cc.
    // Agents draw the position of where they think the ball is
    // Also see example in naobahevior.cc for drawing agent position and
    // orientation.
    
    //worldModel->getRVSender()->clear(); // erases drawings from previous cycle
    //worldModel->getRVSender()->drawPoint("ball", ball.getX(), ball.getY(), 10.0f, RVSender::MAGENTA);
    

    // ### Demo Behaviors ###

    // Walk in different directions
    //return goToTargetRelative(VecPosition(1,0,0), 0); // Forward
    //return goToTargetRelative(VecPosition(-1,0,0), 0); // Backward
    //return goToTargetRelative(VecPosition(0,1,0), 0); // Left
    //return goToTargetRelative(VecPosition(0,-1,0), 0); // Right
    //return goToTargetRelative(VecPosition(1,1,0), 0); // Diagonal
    //return goToTargetRelative(VecPosition(0,1,0), 90); // Turn counter-clockwise
    //return goToTargetRelative(VecPdosition(0,-1,0), -90); // Turn clockwise
    //return goToTargetRelative(VecPosition(1,0,0), 15); // Circle

    // Walk to the ball
    //return goToTarget(ball);

    // Turn in place to face ball
    /*double distance, angle;
    getTargetDistanceAndAngle(ball, distance, angle);
    if (abs(angle) > 10) {
      return goToTargetRelative(VecPosition(), angle);
    } else {
      return SKILL_STAND;
    }*/

    // Walk to ball while always facing forward
    //return goToTargetRelative(worldModel->g2l(ball), -worldModel->getMyAngDeg());

    // Dribble ball toward opponent's goal
    //return kickBall(KICK_DRIBBLE, VecPosition(HALF_FIELD_X, 0, 0));

    // Kick ball toward opponent's goal
    //return kickBall(KICK_FORWARD, VecPosition(HALF_FIELD_X, 0, 0)); // Basic kick
    //return kickBall(KICK_IK, VecPosition(HALF_FIELD_X, 0, 0)); // IK kick

    // Just stand in place
    //return SKILL_STAND;

    // Demo behavior where players form a rotating circle and kick the ball
    // back and forth
    //return demoKickingCircle();
}



//Demo behavior where players form a rotating circle and kick the ball
//back and forth

// SkillType NaoBehavior::demoKickingCircle() {
//     // Parameters for circle
//     VecPosition center = VecPosition(-HALF_FIELD_X/2.0, 0, 0);
//     double circleRadius = 5.0;
//     double rotateRate = 2.5;

//     // Find closest player to ball
//     int playerClosestToBall = -1;
//     double closestDistanceToBall = 10000;
//     for(int i = WO_TEAMMATE1; i < WO_TEAMMATE1+NUM_AGENTS; ++i) {
//         VecPosition temp;
//         int playerNum = i - WO_TEAMMATE1 + 1;
//         if (worldModel->getUNum() == playerNum) {
//             // This is us
//             temp = worldModel->getMyPosition();
//         } else {
//             WorldObject* teammate = worldModel->getWorldObject( i );
//             if (teammate->validPosition) {
//                 temp = teammate->pos;
//             } else {
//                 continue;
//             }
//         }
//         temp.setZ(0);

//         double distanceToBall = temp.getDistanceTo(ball);
//         if (distanceToBall < closestDistanceToBall) {
//             playerClosestToBall = playerNum;
//             closestDistanceToBall = distanceToBall;
//         }
//     }

//     if (playerClosestToBall == worldModel->getUNum()) {
//         // Have closest player kick the ball toward the center
//         return kickBall(KICK_FORWARD, center);
//     } else {
//         // Move to circle position around center and face the center
//         VecPosition localCenter = worldModel->g2l(center);
//         SIM::AngDeg localCenterAngle = atan2Deg(localCenter.getY(), localCenter.getX());

//         // Our desired target position on the circle
//         // Compute target based on uniform number, rotate rate, and time
//         VecPosition target = center + VecPosition(circleRadius,0,0).rotateAboutZ(360.0/(NUM_AGENTS-1)*(worldModel->getUNum()-(worldModel->getUNum() > playerClosestToBall ? 1 : 0)) + worldModel->getTime()*rotateRate);

//         // Adjust target to not be too close to teammates or the ball
//         target = collisionAvoidance(true /*teammate*/, false/*opponent*/, true/*ball*/, 1/*proximity thresh*/, .5/*collision thresh*/, target, true/*keepDistance*/);

//         if (me.getDistanceTo(target) < .25 && abs(localCenterAngle) <= 10) {
//             // Close enough to desired position and orientation so just stand
//             return SKILL_STAND;
//         } else if (me.getDistanceTo(target) < .5) {
//             // Close to desired position so start turning to face center
//             return goToTargetRelative(worldModel->g2l(target), localCenterAngle);
//         } else {
//             // Move toward target location
//             return goToTarget(target);
//         }
//     }
// }



