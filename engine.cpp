// =====================================================================
//  长征 · 从烽火到星辰 —— 游戏核心引擎 (C++ 后端)
//  通信协议：基于行的文本协议，stdin 读取命令，stdout 输出状态
//  编译：g++ -std=c++17 engine.cpp -o engine  (Windows 生成 engine.exe)
// =====================================================================
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <algorithm>

using namespace std;

// ---------- 资源类型 ----------
enum Res {
    TIME = 0,        // 剩余时间 (火星日)
    ENERGY,          // 能源 0-100
    MATERIALS,       // 物资 0-100
    PERSONNEL,       // 人员 (人数)
    MORALE,          // 士气 0-100
    SAFETY,          // 安全 0-100
    PROGRESS,        // 任务进度 0-100
    RES_COUNT
};

const char* RES_NAME[] = {
    "time", "energy", "materials", "personnel", "morale", "safety", "progress"
};

// ---------- 效果：资源变化 ----------
struct Effect {
    Res res;
    int delta;
};

// ---------- 选项 ----------
struct Choice {
    string text;
    vector<Effect> effects;
    string result;   // 选择后的即时描述
};

// ---------- 事件 ----------
struct Event {
    string id;
    int minProgress; // 出现所需最小进度
    int maxProgress; // 出现所需最大进度
    string phase;    // 所属阶段名
    string title;
    string image;    // emoji 图标
    string narrative;
    vector<Choice> choices;
    bool repeatable;
};

// ---------- 游戏状态 ----------
struct GameState {
    int sol;
    int maxSol;
    int res[RES_COUNT];
    string phaseName;
    string currentEventId;
    int currentChoiceCount;
    bool ended;
    string endingTitle;
    string endingEmoji;
    string endingText;
    bool endingWin;
    vector<string> log;
    vector<string> usedEvents;
};

GameState G;
vector<Event> EVENTS;

// ---------- 工具：在 [a,b] 内夹取 ----------
int clamp(int v, int lo, int hi) { return max(lo, min(hi, v)); }

// ---------- 资源名称 -> 中文显示 ----------
string resDisplay(Res r) {
    switch (r) {
        case TIME: return "时间";
        case ENERGY: return "能源";
        case MATERIALS: return "物资";
        case PERSONNEL: return "人员";
        case MORALE: return "士气";
        case SAFETY: return "安全";
        case PROGRESS: return "进度";
    }
    return "?";
}

// ---------- 资源图标 ----------
string resIcon(Res r) {
    switch (r) {
        case TIME: return "⏳";
        case ENERGY: return "⚡";
        case MATERIALS: return "📦";
        case PERSONNEL: return "👥";
        case MORALE: return "❤️";
        case SAFETY: return "🛡";
        case PROGRESS: return "📈";
    }
    return "";
}

// =====================================================================
//  事件库
// =====================================================================
void initEvents() {
    EVENTS.clear();

    // ---------------- 阶段一：星火 (0-25) ----------------
    EVENTS.push_back({
        "e01", 0, 25, "星火", "登陆乌托邦平原", "🛬",
        "着陆器稳稳落在火星乌托邦平原的红色尘埃上。舷窗外，赤红色的荒原一望无际，"
        "天空呈暗粉色。你是\"长征计划\"的指挥官，带领6名机组执行人类首次火星永久基地任务。"
        "基地代号\"瑞金\"——取长征出发地之意。现在，你必须决定第一步行动。",
        {
            {"立即展开太阳能阵列，优先保障能源供应",
                {{TIME,-2},{ENERGY,+30},{PROGRESS,+8}},
                "太阳能板在稀薄的火星大气中展开，阳光转化为稳定的电流。能源系统在线。"},
            {"优先搭建充气式居住舱，确保人员有安身之所",
                {{TIME,-3},{MATERIALS,-10},{SAFETY,+15},{PROGRESS,+8}},
                "居住舱在两小时内充气完成，气密检测通过。机组终于有了家。"},
            {"先派遣地质小组勘察周边水冰分布",
                {{TIME,-2},{MATERIALS,+8},{SAFETY,-5},{PROGRESS,+6}},
                "探测显示基地东北3公里处有丰富的地下冰储量，未来水源有了保障。"}
        },
        false
    });

    EVENTS.push_back({
        "e02", 0, 25, "星火", "能源路线之争", "⚡",
        "能源是一切的基础。火星上太阳能受沙尘暴和昼夜影响巨大，而小型核反应堆"
        "虽然稳定但存在辐射风险。能源工程师林曦提出两套方案，你必须拍板。",
        {
            {"全力铺设太阳能阵列，清洁能源",
                {{TIME,-4},{ENERGY,+25},{MATERIALS,-12},{PROGRESS,+7}},
                "成片的太阳能板在火星阳光下闪烁，日发电量达到设计值的92%。"},
            {"启封小型核反应堆，一劳永逸",
                {{TIME,-3},{ENERGY,+35},{SAFETY,-12},{MORALE,-8},{PROGRESS,+10}},
                "反应堆达到临界状态，源源不断的电力输入基地电网。但机组对辐射有些担忧。"},
            {"混合方案：太阳能为主，核电备用",
                {{TIME,-5},{ENERGY,+28},{MATERIALS,-8},{PROGRESS,+8}},
                "双能源系统搭建完成，互为备份。虽然耗时，但容错率大大提高。"}
        },
        false
    });

    EVENTS.push_back({
        "e03", 0, 25, "星火", "水冰开采", "💧",
        "水是生命之源，也是推进剂的原料。基地地下冰储量丰富，但开采设备需要调试。"
        "开采过程中会消耗大量能源，并产生粉尘污染风险。",
        {
            {"全负荷开采，尽快储备水资源",
                {{TIME,-3},{ENERGY,-15},{MATERIALS,+20},{SAFETY,-6},{PROGRESS,+6}},
                "水冰源源不断地被提取、净化，储水罐液位持续上升。"},
            {"稳步开采，控制粉尘与能耗",
                {{TIME,-5},{ENERGY,-8},{MATERIALS,+14},{SAFETY,+3},{PROGRESS,+4}},
                "虽然进度慢一些，但基地环境保持清洁，机组健康无虞。"},
            {"先建造水处理设施再开采",
                {{TIME,-6},{MATERIALS,-6},{ENERGY,-5},{MATERIALS,+22},{PROGRESS,+5}},
                "闭环水处理系统上线，未来每一滴水都将被循环利用。"}
        },
        false
    });

    EVENTS.push_back({
        "e04", 10, 30, "星火", "火星第一夜", "🌙",
        "夜幕降临火星。气温骤降至零下80摄氏度，基地首次全靠自身系统维持生存。"
        "这是对所有系统的终极考验，也是机组心理的第一道坎。",
        {
            {"全员在岗，密切监控所有系统",
                {{TIME,-1},{MORALE,-5},{SAFETY,+10},{ENERGY,-5}},
                "漫长的黑夜过去了，所有系统运行正常。机组虽疲惫但信心大增。"},
            {"安排轮班休息，保持战斗力",
                {{TIME,-1},{MORALE,+8},{SAFETY,-5}},
                "充足的休息让机组精神焕发，但夜间无人值守的时段让人有些不安。"},
            {"举办一个小型聚会，缓解紧张情绪",
                {{TIME,-1},{MORALE,+15},{ENERGY,-8}},
                "在遥远的火星上，笑声回荡在居住舱里。长征精神，从乐观开始。"}
        },
        false
    });

    // ---------------- 阶段二：砥砺 (25-50) ----------------
    EVENTS.push_back({
        "e05", 20, 45, "砥砺", "沙尘暴来袭", "🌪️",
        "监测站发出警报：一场区域性沙尘暴正在逼近，预计持续5-8个火星日。"
        "沙尘将遮蔽阳光、损坏外露设备、甚至威胁舱体密封。",
        {
            {"紧急回收所有外露设备，加固舱体",
                {{TIME,-4},{ENERGY,-12},{MATERIALS,-8},{SAFETY,+12}},
                "设备全部回收，舱体加固完毕。基地在风暴中稳如磐石。"},
            {"只回收关键设备，全力保护太阳能板",
                {{TIME,-3},{ENERGY,+5},{SAFETY,-8},{MATERIALS,-5}},
                "太阳能板得到保护，但部分外露设备在沙尘中受损。"},
            {"全员进入避难模式，听天由命",
                {{TIME,-6},{SAFETY,-15},{MORALE,-10}},
                "风暴过去了，基地多处受损。机组在黑暗中度过了难熬的几天。"}
        },
        false
    });

    EVENTS.push_back({
        "e06", 20, 45, "砥砺", "深空通讯中断", "📡",
        "与地球的深空通讯突然中断。技术排查发现是高增益天线的伺服机构故障。"
        "没有地球的支持，所有决策都必须由你独立做出——这正是\"长征\"的意义。",
        {
            {"立即组织抢修，恢复通讯",
                {{TIME,-3},{MATERIALS,-10},{ENERGY,-8},{PROGRESS,-3}},
                "经过48小时奋战，天线重新对准地球。来自家乡的信号让人热泪盈眶。"},
            {"启用备用低增益天线，维持基本通讯",
                {{TIME,-1},{ENERGY,-5},{MATERIALS,-3}},
                "虽然带宽极低，但至少能向地球报平安。决策仍需自主。"},
            {"借此机会完全自主运行，锻炼团队",
                {{TIME,-2},{MORALE,+10},{SAFETY,-5},{PROGRESS,+5}},
                "机组展现了惊人的自主性。没有后方，他们就是自己的后方。"}
        },
        false
    });

    EVENTS.push_back({
        "e07", 20, 45, "砥砺", "医疗紧急情况", "🏥",
        "地质学家陈雪在外出采样时不慎摔倒，疑似骨折。火星上没有医院，"
        "只有随队医生张磊和有限的医疗物资。如何处理考验着你的决断。",
        {
            {"立即返回基地进行手术固定",
                {{TIME,-2},{MATERIALS,-12},{SAFETY,+8},{PERSONNEL,0}},
                "张磊医生在简陋条件下完成了固定手术，陈雪需要休养但无生命危险。"},
            {"就地简易固定，继续完成采样任务",
                {{TIME,-1},{MATERIALS,-4},{SAFETY,-10},{MORALE,-8}},
                "采样数据带回了，但陈雪的伤势恶化了。机组情绪低落。"},
            {"使用珍贵的3D打印设备制造外骨骼支架",
                {{TIME,-3},{MATERIALS,-18},{ENERGY,-10},{SAFETY,+12}},
                "定制化的外骨骼支架完美贴合，陈雪很快就能参与轻度工作。科技改变命运。"}
        },
        false
    });

    EVENTS.push_back({
        "e08", 25, 50, "砥砺", "物资短缺警报", "📉",
        "后勤官报告：按照当前消耗速度，关键物资将在任务中期出现缺口。"
        "必须开源节流。你会优先削减哪方面的消耗？",
        {
            {"削减非关键科研任务，保障生存物资",
                {{TIME,-1},{MATERIALS,+15},{PROGRESS,-6},{MORALE,-5}},
                "物资压力缓解了，但科研团队对搁置的实验感到遗憾。"},
            {"厉行节约，全方位压缩消耗",
                {{TIME,-1},{MATERIALS,+10},{MORALE,-8},{SAFETY,-3}},
                "每一克物资都被精打细算。机组虽然辛苦，但物资储备趋于稳定。"},
            {"组织一次远程物资搜寻，利用当地资源",
                {{TIME,-4},{MATERIALS,+22},{SAFETY,-10},{ENERGY,-10}},
                "远征队带回了大量可用矿物和水冰，但长途跋涉让所有人精疲力竭。"}
        },
        false
    });

    // ---------------- 阶段三：转折 (45-70) ----------------
    EVENTS.push_back({
        "e09", 40, 65, "转折", "团队分歧", "⚖️",
        "关于下一步的重点，机组产生了严重分歧：工程师主张优先扩建基地设施，"
        "科学家坚持推进生命探测实验，医生则呼吁关注全员健康与休整。"
        "作为指挥官，你必须做出选择——正如长征路上的每一次抉择。",
        {
            {"支持工程师：先把基地建牢固",
                {{MATERIALS,-10},{PROGRESS,+12},{MORALE,-5}},
                "基地扩建工程启动，设施更加完善。但科学家们有些失落。"},
            {"支持科学家：追寻生命的答案",
                {{TIME,-2},{MATERIALS,-8},{PROGRESS,+8},{MORALE,+5},{ENERGY,-8}},
                "生命探测实验全面展开。对未知的渴望点燃了机组的热情。"},
            {"支持医生：以人为本，先休整再出发",
                {{TIME,-3},{MORALE,+20},{SAFETY,+12},{PROGRESS,-2}},
                "机组得到了充分休整，状态焕然一新。磨刀不误砍柴工。"},
            {"折中方案：三线并行，各分配有限资源",
                {{MATERIALS,-12},{ENERGY,-10},{PROGRESS,+5},{MORALE,+3}},
                "虽然每条线都资源紧张，但各方都被顾及，团队保持了团结。"}
        },
        false
    });

    EVENTS.push_back({
        "e10", 40, 65, "转折", "太阳耀斑警报", "☀️",
        "太阳监测系统发出红色警报：一次强太阳耀斑正在向火星方向喷发，"
        "预计12小时内到达。高能粒子将严重威胁暴露在外的人员和电子设备。",
        {
            {"全员进入辐射避难所，关闭非必要系统",
                {{TIME,-2},{ENERGY,-15},{SAFETY,+20},{MORALE,-5}},
                "耀斑如期而至，基地在避难所的保护下安然无恙。机组目睹了壮丽的极光。"},
            {"紧急召回外出人员，加固电子设备屏蔽",
                {{TIME,-1},{MATERIALS,-8},{ENERGY,-10},{SAFETY,+10}},
                "大部分设备得到保护，但仍有部分传感器损坏。"},
            {"抓住机会进行太阳物理观测，这是难得的数据",
                {{TIME,-1},{SAFETY,-20},{PROGRESS,+15},{MORALE,+8}},
                "机组获取了极其珍贵的第一手耀斑数据，但承受了相当的辐射剂量。"}
        },
        false
    });

    EVENTS.push_back({
        "e11", 45, 70, "转折", "意外发现", "🔬",
        "生命探测实验取得惊人发现：在基地附近的地下冰层样本中，"
        "检测到了疑似有机大分子的信号。这可能是人类历史上最伟大的发现，"
        "但也可能只是地球污染。是否公布？",
        {
            {"立即向地球报告，寻求国际合作验证",
                {{TIME,-2},{PROGRESS,+12},{MORALE,+10},{SAFETY,-3}},
                "消息传回地球，全球震动。国际同行对样本展开联合分析。"},
            {"先内部反复验证，确认后再公布",
                {{TIME,-5},{PROGRESS,+8},{MATERIALS,-6}},
                "经过严苛的验证流程，结果依然指向有机起源。公布时更有底气。"},
            {"暂时保密，避免引发误报和舆论风险",
                {{TIME,-1},{PROGRESS,+3},{MORALE,-8}},
                "机组对隐瞒发现感到不安。但科学的严谨性要求他们再等等。"}
        },
        false
    });

    // ---------------- 阶段四：攻坚 (65-88) ----------------
    EVENTS.push_back({
        "e12", 60, 85, "攻坚", "温室工程", "🌱",
        "要实现基地自给自足，必须建立受控生态生命保障系统。"
        "温室工程是整个任务的关键节点——它将决定人类能否在火星上持续生存。",
        {
            {"建造全功能温室，种植多种作物",
                {{TIME,-6},{MATERIALS,-20},{ENERGY,-15},{PROGRESS,+18},{MORALE,+10}},
                "第一株绿色植物在火星土壤中发芽。这是人类文明的又一粒种子。"},
            {"先建小型实验温室，验证技术可行性",
                {{TIME,-4},{MATERIALS,-10},{ENERGY,-8},{PROGRESS,+10}},
                "小型温室运行良好，为后续扩建积累了宝贵经验。"},
            {"引入藻类生物反应器，高效产氧产食",
                {{TIME,-5},{MATERIALS,-14},{ENERGY,-12},{PROGRESS,+15}},
                "藻类反应器效率惊人，氧气和蛋白质产出超出预期。"}
        },
        false
    });

    EVENTS.push_back({
        "e13", 60, 85, "攻坚", "漫游车远征", "🚙",
        "为了勘探基地周边50公里范围内的资源分布，需要组织一次远程漫游车远征。"
        "这是对机组体力、设备可靠性和导航技术的综合考验。",
        {
            {"双人小队，轻装快速侦察",
                {{TIME,-4},{ENERGY,-12},{MATERIALS,+15},{SAFETY,-8},{PROGRESS,+8}},
                "远征队带回了详细的资源分布图，发现了一处高浓度矿物露头。"},
            {"四人完整科考队，全面采样",
                {{TIME,-7},{ENERGY,-20},{MATERIALS,+25},{SAFETY,-12},{PROGRESS,+14}},
                "海量的样本和数据让基地的资源储备翻了一番，但代价是巨大的体力消耗。"},
            {"使用自主漫游无人机，零人员风险",
                {{TIME,-3},{MATERIALS,-10},{ENERGY,-15},{PROGRESS,+6}},
                "无人机高效完成了测绘，虽然深度采样不足，但保证了人员安全。"}
        },
        false
    });

    EVENTS.push_back({
        "e14", 65, 88, "攻坚", "反应堆维护", "⚛️",
        "小型核反应堆的运行时间已接近设计寿命的一半，需要进行一次深度维护。"
        "这涉及高辐射环境作业，必须谨慎安排。",
        {
            {"由受过专项训练的工程师进行手动维护",
                {{TIME,-3},{SAFETY,-15},{ENERGY,+20},{PROGRESS,+6},{MORALE,-5}},
                "维护完美完成，反应堆效率恢复到出厂水平。工程师承受了一定辐射剂量。"},
            {"使用遥控机器人进行维护",
                {{TIME,-5},{MATERIALS,-12},{ENERGY,+15},{SAFETY,+5}},
                "虽然耗时更长，但人员零接触辐射。安全性是长征的底线。"},
            {"推迟维护，等任务后期再说",
                {{TIME,-1},{SAFETY,-18},{ENERGY,-8}},
                "反应堆开始出现效率衰减。隐患被推迟了，但没有消失。"}
        },
        false
    });

    // ---------------- 阶段五：星辰 (85-100) ----------------
    EVENTS.push_back({
        "e15", 82, 96, "星辰", "自给自足验证", "🏗️",
        "地球评估委员会要求基地进行为期两周的\"完全自给自足验证\"："
        "切断所有来自地球的补给模拟，测试基地能否独立维持运转。",
        {
            {"严格执行标准流程，逐项验证所有子系统",
                {{TIME,-5},{ENERGY,-12},{MATERIALS,-8},{SAFETY,+8},{PROGRESS,+12}},
                "所有系统通过验证。基地可以在不依赖地球的情况下持续运行。"},
            {"简化流程，重点验证核心生存系统",
                {{TIME,-3},{ENERGY,-6},{PROGRESS,+8},{SAFETY,-5}},
                "核心系统验证通过，但部分子系统的数据不够完整。"},
            {"边验证边优化，发现问题立即改进",
                {{TIME,-6},{MATERIALS,-10},{ENERGY,-10},{PROGRESS,+15},{MORALE,+5}},
                "验证中发现并修复了3个潜在问题。基地比验证前更强大了。"}
        },
        false
    });

    EVENTS.push_back({
        "e16", 90, 100, "星辰", "向地球报告", "📡",
        "自给自足验证通过，\"瑞金\"基地正式成为人类在火星的第一个永久据点。"
        "现在，你需要向地球提交最终任务报告。这份报告将写入人类史册。",
        {
            {"如实汇报所有数据，包括失误和教训",
                {{PROGRESS,+8},{MORALE,+8}},
                "地球指挥部高度评价了报告的坦诚与专业。失误是未来成功的基石。"},
            {"重点突出成就，弱化困难",
                {{PROGRESS,+6},{MORALE,+3},{SAFETY,-3}},
                "报告赢得了一片赞誉，但有经验的人看出了其中的回避。"},
            {"以\"长征精神\"为主线，讲述团队的故事",
                {{PROGRESS,+7},{MORALE,+15}},
                "报告感动了无数人。从瑞金到火星，长征的火种永不熄灭。"}
        },
        false
    });
}

// =====================================================================
//  游戏逻辑
// =====================================================================
void initGame() {
    G.sol = 1;
    G.maxSol = 180;
    G.res[TIME] = 180;
    G.res[ENERGY] = 70;
    G.res[MATERIALS] = 60;
    G.res[PERSONNEL] = 6;
    G.res[MORALE] = 80;
    G.res[SAFETY] = 85;
    G.res[PROGRESS] = 0;
    G.phaseName = "星火";
    G.ended = false;
    G.log.clear();
    G.usedEvents.clear();
    G.currentEventId = "";
    G.log.push_back("任务启动：长征计划 · 火星永久基地建立");
}

// 根据进度获取阶段名
string phaseForProgress(int p) {
    if (p < 25) return "星火";
    if (p < 50) return "砥砺";
    if (p < 75) return "转折";
    if (p < 90) return "攻坚";
    return "星辰";
}

// 选择下一个事件
Event* pickNextEvent() {
    int p = G.res[PROGRESS];
    vector<Event*> candidates;
    for (auto& e : EVENTS) {
        if (p >= e.minProgress && p <= e.maxProgress) {
            bool used = false;
            for (auto& id : G.usedEvents) if (id == e.id) { used = true; break; }
            if (!used || e.repeatable) candidates.push_back(&e);
        }
    }
    if (candidates.empty()) {
        // 如果本阶段事件用完，放宽到任意未用事件
        for (auto& e : EVENTS) {
            bool used = false;
            for (auto& id : G.usedEvents) if (id == e.id) { used = true; break; }
            if (!used) candidates.push_back(&e);
        }
    }
    if (candidates.empty()) return nullptr;
    int idx = rand() % candidates.size();
    return candidates[idx];
}

// 应用选项效果
string applyEffects(const Choice& c) {
    stringstream ss;
    for (auto& ef : c.effects) {
        G.res[ef.res] = clamp(G.res[ef.res] + ef.delta, 0, (ef.res == PERSONNEL) ? 6 : 100);
        if (ef.res == TIME) {
            // 时间消耗换算为 sol 推进
            G.sol += (-ef.delta);
            G.res[TIME] = max(0, G.maxSol - G.sol + 1);
        }
    }
    return c.result;
}

// 检查结局
bool checkEnding() {
    if (G.res[PROGRESS] >= 100) {
        G.ended = true;
        G.endingWin = true;
        G.endingTitle = "任务达成 · 星火燎原";
        G.endingEmoji = "🌠";
        G.endingText =
            "在第 " + to_string(G.sol) + " 个火星日，\"瑞金\"基地达成全部建设目标。"
            "人类在火星上拥有了第一个可以自给自足的永久据点。\n\n"
            "从1934年的瑞金出发，到2047年的火星\"瑞金\"——两万五千里的足迹，"
            "最终延伸到了星辰大海。长征从未结束，它只是换了一种方式继续。\n\n"
            "你的名字将被刻在火星的岩石上，与那些红米饭、南瓜汤的故事一起，"
            "成为人类不屈精神的永恒注脚。";
        return true;
    }
    if (G.res[TIME] <= 0) {
        G.ended = true;
        G.endingWin = false;
        G.endingTitle = "任务失败 · 窗口关闭";
        G.endingEmoji = "⌛";
        G.endingText =
            "地火合相时刻到来，180个火星日的窗口关闭了。基地尚未达到自给自足标准，"
            "与地球的通讯即将中断12周。在没有后援的火星上，机组面临着严酷的生存考验。\n\n"
            "长征路上，时间从来都是最无情的敌人。";
        return true;
    }
    if (G.res[PERSONNEL] <= 0) {
        G.ended = true;
        G.endingWin = false;
        G.endingTitle = "任务失败 · 牺牲";
        G.endingEmoji = "🕯️";
        G.endingText =
            "最后一名机组人员倒下了。\"瑞金\"基地成为了火星上一座静默的纪念碑。\n\n"
            "他们的名字将被永远铭记——正如长征路上那些没有走到终点的战士。"
            "他们的牺牲，为后来者照亮了道路。";
        return true;
    }
    if (G.res[SAFETY] <= 0) {
        G.ended = true;
        G.endingWin = false;
        G.endingTitle = "任务失败 · 基地失守";
        G.endingEmoji = "💥";
        G.endingText =
            "一场灾难性的系统故障击穿了基地的安全防线。居住舱失压，"
            "机组在最后时刻启动了救生程序。\n\n"
            "长征从来不是坦途。失败不是终点，而是下一次出发的起点。";
        return true;
    }
    if (G.res[MORALE] <= 0) {
        G.ended = true;
        G.endingWin = false;
        G.endingTitle = "任务失败 · 精神崩溃";
        G.endingEmoji = "😔";
        G.endingText =
            "持续的高压、孤独和挫折击垮了机组的精神防线。他们请求紧急撤离。\n\n"
            "长征不仅是身体的远征，更是精神的远征。没有坚定的信念，"
            "再先进的技术也无法让人在绝境中坚持下去。";
        return true;
    }
    return false;
}

// =====================================================================
//  通信输出
// =====================================================================
void out(const string& k, const string& v) {
    cout << "FIELD " << k << " " << v << endl;
}

void emitEvent(Event* e) {
    G.currentEventId = e->id;
    G.currentChoiceCount = (int)e->choices.size();
    out("sol", to_string(G.sol));
    out("phase", e->phase);
    out("progress", to_string(G.res[PROGRESS]));
    out("time", to_string(G.res[TIME]));
    out("energy", to_string(G.res[ENERGY]));
    out("materials", to_string(G.res[MATERIALS]));
    out("personnel", to_string(G.res[PERSONNEL]));
    out("morale", to_string(G.res[MORALE]));
    out("safety", to_string(G.res[SAFETY]));
    out("title", e->title);
    out("image", e->image);
    out("narrative", e->narrative);

    for (int i = 0; i < (int)e->choices.size(); i++) {
        stringstream cost;
        for (auto& ef : e->choices[i].effects) {
            cost << resIcon(ef.res);
            if (ef.delta > 0) cost << "+";
            cost << ef.delta << " ";
        }
        cout << "CHOICE " << i << " " << e->choices[i].text << endl;
        cout << "COST " << i << " " << cost.str() << endl;
    }
    cout << "END" << endl;
}

void emitEnding() {
    out("sol", to_string(G.sol));
    out("progress", to_string(G.res[PROGRESS]));
    out("time", to_string(G.res[TIME]));
    out("energy", to_string(G.res[ENERGY]));
    out("materials", to_string(G.res[MATERIALS]));
    out("personnel", to_string(G.res[PERSONNEL]));
    out("morale", to_string(G.res[MORALE]));
    out("safety", to_string(G.res[SAFETY]));
    out("ending_title", G.endingTitle);
    out("ending_emoji", G.endingEmoji);
    out("ending_text", G.endingText);
    out("ending_win", G.endingWin ? "1" : "0");
    cout << "END" << endl;
}

void emitResult(Event* e, int choiceIdx) {
    // 输出选择结果描述 + 更新后状态（用于日志和过渡）
    out("result", e->choices[choiceIdx].result);
    cout << "END" << endl;
}

// =====================================================================
//  主循环
// =====================================================================
int main() {
    srand((unsigned)time(nullptr));
    initEvents();

    string line;
    while (getline(cin, line)) {
        // 去除回车
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (line == "START") {
            initGame();
            Event* e = pickNextEvent();
            if (e) {
                G.usedEvents.push_back(e->id);
                emitEvent(e);
            } else {
                emitEnding();
            }
        }
        else if (line.substr(0, 7) == "CHOICE ") {
            int idx = atoi(line.c_str() + 7);
            // 找到当前事件
            Event* cur = nullptr;
            for (auto& e : EVENTS) if (e.id == G.currentEventId) { cur = &e; break; }
            if (!cur || idx < 0 || idx >= (int)cur->choices.size()) {
                cout << "ERROR invalid choice" << endl;
                cout << "END" << endl;
                continue;
            }
            // 应用效果
            string result = applyEffects(cur->choices[idx]);
            G.log.push_back("[SOL " + to_string(G.sol) + "] " + cur->title + " → " + cur->choices[idx].text);
            // 推进时间：每个选择额外消耗1天（执行时间）
            G.sol += 1;
            G.res[TIME] = max(0, G.maxSol - G.sol + 1);
            // 更新阶段名
            G.phaseName = phaseForProgress(G.res[PROGRESS]);

            // 先输出结果描述
            emitResult(cur, idx);

            // 等待前端发送 NEXT 以继续
        }
        else if (line == "NEXT") {
            if (checkEnding()) {
                emitEnding();
            } else {
                Event* e = pickNextEvent();
                if (e) {
                    G.usedEvents.push_back(e->id);
                    emitEvent(e);
                } else {
                    // 所有事件用完，强制完成或失败
                    if (G.res[PROGRESS] >= 100) {
                        G.res[PROGRESS] = 100;
                    }
                    checkEnding();
                    emitEnding();
                }
            }
        }
        else if (line == "QUIT") {
            break;
        }
    }
    return 0;
}
