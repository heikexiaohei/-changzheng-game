# =====================================================================
#  长征 · 从烽火到星辰 —— 游戏核心引擎 (Python 后端，与 C++ 版等价)
#  通信协议：基于行的文本协议，stdin 读取命令，stdout 输出状态
#  运行：python engine.py
# =====================================================================
import sys
import random

# 资源类型
TIME, ENERGY, MATERIALS, PERSONNEL, MORALE, SAFETY, PROGRESS = range(7)
RES_COUNT = 7

RES_NAME = ["time", "energy", "materials", "personnel", "morale", "safety", "progress"]

def res_display(r):
    return ["时间","能源","物资","人员","士气","安全","进度"][r]

def res_icon(r):
    return ["⏳","⚡","📦","👥","❤️","🛡","📈"][r]

def clamp(v, lo, hi):
    return max(lo, min(hi, v))

class Effect:
    def __init__(self, res, delta):
        self.res = res
        self.delta = delta

class Choice:
    def __init__(self, text, effects, result):
        self.text = text
        self.effects = effects
        self.result = result

class Event:
    def __init__(self, id, minp, maxp, phase, title, image, narrative, choices, repeatable=False):
        self.id = id
        self.minProgress = minp
        self.maxProgress = maxp
        self.phase = phase
        self.title = title
        self.image = image
        self.narrative = narrative
        self.choices = choices
        self.repeatable = repeatable

class GameState:
    def __init__(self):
        self.sol = 1
        self.maxSol = 180
        self.res = [180, 70, 60, 6, 80, 85, 0]
        self.phaseName = "星火"
        self.currentEventId = ""
        self.currentChoiceCount = 0
        self.ended = False
        self.endingTitle = ""
        self.endingEmoji = ""
        self.endingText = ""
        self.endingWin = False
        self.log = []
        self.usedEvents = []

G = GameState()
EVENTS = []

def init_events():
    global EVENTS
    EVENTS = []
    EVENTS.append(Event("e01",0,25,"星火","登陆乌托邦平原","🛬",
        "着陆器稳稳落在火星乌托邦平原的红色尘埃上。舷窗外，赤红色的荒原一望无际，"
        "天空呈暗粉色。你是\"长征计划\"的指挥官，带领6名机组执行人类首次火星永久基地任务。"
        "基地代号\"瑞金\"——取长征出发地之意。现在，你必须决定第一步行动。",
        [Choice("立即展开太阳能阵列，优先保障能源供应",
                [Effect(TIME,-2),Effect(ENERGY,+30),Effect(PROGRESS,+8)],
                "太阳能板在稀薄的火星大气中展开，阳光转化为稳定的电流。能源系统在线。"),
         Choice("优先搭建充气式居住舱，确保人员有安身之所",
                [Effect(TIME,-3),Effect(MATERIALS,-10),Effect(SAFETY,+15),Effect(PROGRESS,+8)],
                "居住舱在两小时内充气完成，气密检测通过。机组终于有了家。"),
         Choice("先派遣地质小组勘察周边水冰分布",
                [Effect(TIME,-2),Effect(MATERIALS,+8),Effect(SAFETY,-5),Effect(PROGRESS,+6)],
                "探测显示基地东北3公里处有丰富的地下冰储量，未来水源有了保障。")],False))

    EVENTS.append(Event("e02",0,25,"星火","能源路线之争","⚡",
        "能源是一切的基础。火星上太阳能受沙尘暴和昼夜影响巨大，而小型核反应堆"
        "虽然稳定但存在辐射风险。能源工程师林曦提出两套方案，你必须拍板。",
        [Choice("全力铺设太阳能阵列，清洁能源",
                [Effect(TIME,-4),Effect(ENERGY,+25),Effect(MATERIALS,-12),Effect(PROGRESS,+7)],
                "成片的太阳能板在火星阳光下闪烁，日发电量达到设计值的92%。"),
         Choice("启封小型核反应堆，一劳永逸",
                [Effect(TIME,-3),Effect(ENERGY,+35),Effect(SAFETY,-12),Effect(MORALE,-8),Effect(PROGRESS,+10)],
                "反应堆达到临界状态，源源不断的电力输入基地电网。但机组对辐射有些担忧。"),
         Choice("混合方案：太阳能为主，核电备用",
                [Effect(TIME,-5),Effect(ENERGY,+28),Effect(MATERIALS,-8),Effect(PROGRESS,+8)],
                "双能源系统搭建完成，互为备份。虽然耗时，但容错率大大提高。")],False))

    EVENTS.append(Event("e03",0,25,"星火","水冰开采","💧",
        "水是生命之源，也是推进剂的原料。基地地下冰储量丰富，但开采设备需要调试。"
        "开采过程中会消耗大量能源，并产生粉尘污染风险。",
        [Choice("全负荷开采，尽快储备水资源",
                [Effect(TIME,-3),Effect(ENERGY,-15),Effect(MATERIALS,+20),Effect(SAFETY,-6),Effect(PROGRESS,+6)],
                "水冰源源不断地被提取、净化，储水罐液位持续上升。"),
         Choice("稳步开采，控制粉尘与能耗",
                [Effect(TIME,-5),Effect(ENERGY,-8),Effect(MATERIALS,+14),Effect(SAFETY,+3),Effect(PROGRESS,+4)],
                "虽然进度慢一些，但基地环境保持清洁，机组健康无虞。"),
         Choice("先建造水处理设施再开采",
                [Effect(TIME,-6),Effect(MATERIALS,-6),Effect(ENERGY,-5),Effect(MATERIALS,+22),Effect(PROGRESS,+5)],
                "闭环水处理系统上线，未来每一滴水都将被循环利用。")],False))

    EVENTS.append(Event("e04",10,30,"星火","火星第一夜","🌙",
        "夜幕降临火星。气温骤降至零下80摄氏度，基地首次全靠自身系统维持生存。"
        "这是对所有系统的终极考验，也是机组心理的第一道坎。",
        [Choice("全员在岗，密切监控所有系统",
                [Effect(TIME,-1),Effect(MORALE,-5),Effect(SAFETY,+10),Effect(ENERGY,-5)],
                "漫长的黑夜过去了，所有系统运行正常。机组虽疲惫但信心大增。"),
         Choice("安排轮班休息，保持战斗力",
                [Effect(TIME,-1),Effect(MORALE,+8),Effect(SAFETY,-5)],
                "充足的休息让机组精神焕发，但夜间无人值守的时段让人有些不安。"),
         Choice("举办一个小型聚会，缓解紧张情绪",
                [Effect(TIME,-1),Effect(MORALE,+15),Effect(ENERGY,-8)],
                "在遥远的火星上，笑声回荡在居住舱里。长征精神，从乐观开始。")],False))

    EVENTS.append(Event("e05",20,45,"砥砺","沙尘暴来袭","🌪️",
        "监测站发出警报：一场区域性沙尘暴正在逼近，预计持续5-8个火星日。"
        "沙尘将遮蔽阳光、损坏外露设备、甚至威胁舱体密封。",
        [Choice("紧急回收所有外露设备，加固舱体",
                [Effect(TIME,-4),Effect(ENERGY,-12),Effect(MATERIALS,-8),Effect(SAFETY,+12)],
                "设备全部回收，舱体加固完毕。基地在风暴中稳如磐石。"),
         Choice("只回收关键设备，全力保护太阳能板",
                [Effect(TIME,-3),Effect(ENERGY,+5),Effect(SAFETY,-8),Effect(MATERIALS,-5)],
                "太阳能板得到保护，但部分外露设备在沙尘中受损。"),
         Choice("全员进入避难模式，听天由命",
                [Effect(TIME,-6),Effect(SAFETY,-15),Effect(MORALE,-10)],
                "风暴过去了，基地多处受损。机组在黑暗中度过了难熬的几天。")],False))

    EVENTS.append(Event("e06",20,45,"砥砺","深空通讯中断","📡",
        "与地球的深空通讯突然中断。技术排查发现是高增益天线的伺服机构故障。"
        "没有地球的支持，所有决策都必须由你独立做出——这正是\"长征\"的意义。",
        [Choice("立即组织抢修，恢复通讯",
                [Effect(TIME,-3),Effect(MATERIALS,-10),Effect(ENERGY,-8),Effect(PROGRESS,-3)],
                "经过48小时奋战，天线重新对准地球。来自家乡的信号让人热泪盈眶。"),
         Choice("启用备用低增益天线，维持基本通讯",
                [Effect(TIME,-1),Effect(ENERGY,-5),Effect(MATERIALS,-3)],
                "虽然带宽极低，但至少能向地球报平安。决策仍需自主。"),
         Choice("借此机会完全自主运行，锻炼团队",
                [Effect(TIME,-2),Effect(MORALE,+10),Effect(SAFETY,-5),Effect(PROGRESS,+5)],
                "机组展现了惊人的自主性。没有后方，他们就是自己的后方。")],False))

    EVENTS.append(Event("e07",20,45,"砥砺","医疗紧急情况","🏥",
        "地质学家陈雪在外出采样时不慎摔倒，疑似骨折。火星上没有医院，"
        "只有随队医生张磊和有限的医疗物资。如何处理考验着你的决断。",
        [Choice("立即返回基地进行手术固定",
                [Effect(TIME,-2),Effect(MATERIALS,-12),Effect(SAFETY,+8)],
                "张磊医生在简陋条件下完成了固定手术，陈雪需要休养但无生命危险。"),
         Choice("就地简易固定，继续完成采样任务",
                [Effect(TIME,-1),Effect(MATERIALS,-4),Effect(SAFETY,-10),Effect(MORALE,-8)],
                "采样数据带回了，但陈雪的伤势恶化了。机组情绪低落。"),
         Choice("使用珍贵的3D打印设备制造外骨骼支架",
                [Effect(TIME,-3),Effect(MATERIALS,-18),Effect(ENERGY,-10),Effect(SAFETY,+12)],
                "定制化的外骨骼支架完美贴合，陈雪很快就能参与轻度工作。科技改变命运。")],False))

    EVENTS.append(Event("e08",25,50,"砥砺","物资短缺警报","📉",
        "后勤官报告：按照当前消耗速度，关键物资将在任务中期出现缺口。"
        "必须开源节流。你会优先削减哪方面的消耗？",
        [Choice("削减非关键科研任务，保障生存物资",
                [Effect(TIME,-1),Effect(MATERIALS,+15),Effect(PROGRESS,-6),Effect(MORALE,-5)],
                "物资压力缓解了，但科研团队对搁置的实验感到遗憾。"),
         Choice("厉行节约，全方位压缩消耗",
                [Effect(TIME,-1),Effect(MATERIALS,+10),Effect(MORALE,-8),Effect(SAFETY,-3)],
                "每一克物资都被精打细算。机组虽然辛苦，但物资储备趋于稳定。"),
         Choice("组织一次远程物资搜寻，利用当地资源",
                [Effect(TIME,-4),Effect(MATERIALS,+22),Effect(SAFETY,-10),Effect(ENERGY,-10)],
                "远征队带回了大量可用矿物和水冰，但长途跋涉让所有人精疲力竭。")],False))

    EVENTS.append(Event("e09",40,65,"转折","团队分歧","⚖️",
        "关于下一步的重点，机组产生了严重分歧：工程师主张优先扩建基地设施，"
        "科学家坚持推进生命探测实验，医生则呼吁关注全员健康与休整。"
        "作为指挥官，你必须做出选择——正如长征路上的每一次抉择。",
        [Choice("支持工程师：先把基地建牢固",
                [Effect(MATERIALS,-10),Effect(PROGRESS,+12),Effect(MORALE,-5)],
                "基地扩建工程启动，设施更加完善。但科学家们有些失落。"),
         Choice("支持科学家：追寻生命的答案",
                [Effect(TIME,-2),Effect(MATERIALS,-8),Effect(PROGRESS,+8),Effect(MORALE,+5),Effect(ENERGY,-8)],
                "生命探测实验全面展开。对未知的渴望点燃了机组的热情。"),
         Choice("支持医生：以人为本，先休整再出发",
                [Effect(TIME,-3),Effect(MORALE,+20),Effect(SAFETY,+12),Effect(PROGRESS,-2)],
                "机组得到了充分休整，状态焕然一新。磨刀不误砍柴工。"),
         Choice("折中方案：三线并行，各分配有限资源",
                [Effect(MATERIALS,-12),Effect(ENERGY,-10),Effect(PROGRESS,+5),Effect(MORALE,+3)],
                "虽然每条线都资源紧张，但各方都被顾及，团队保持了团结。")],False))

    EVENTS.append(Event("e10",40,65,"转折","太阳耀斑警报","☀️",
        "太阳监测系统发出红色警报：一次强太阳耀斑正在向火星方向喷发，"
        "预计12小时内到达。高能粒子将严重威胁暴露在外的人员和电子设备。",
        [Choice("全员进入辐射避难所，关闭非必要系统",
                [Effect(TIME,-2),Effect(ENERGY,-15),Effect(SAFETY,+20),Effect(MORALE,-5)],
                "耀斑如期而至，基地在避难所的保护下安然无恙。机组目睹了壮丽的极光。"),
         Choice("紧急召回外出人员，加固电子设备屏蔽",
                [Effect(TIME,-1),Effect(MATERIALS,-8),Effect(ENERGY,-10),Effect(SAFETY,+10)],
                "大部分设备得到保护，但仍有部分传感器损坏。"),
         Choice("抓住机会进行太阳物理观测，这是难得的数据",
                [Effect(TIME,-1),Effect(SAFETY,-20),Effect(PROGRESS,+15),Effect(MORALE,+8)],
                "机组获取了极其珍贵的第一手耀斑数据，但承受了相当的辐射剂量。")],False))

    EVENTS.append(Event("e11",45,70,"转折","意外发现","🔬",
        "生命探测实验取得惊人发现：在基地附近的地下冰层样本中，"
        "检测到了疑似有机大分子的信号。这可能是人类历史上最伟大的发现，"
        "但也可能只是地球污染。是否公布？",
        [Choice("立即向地球报告，寻求国际合作验证",
                [Effect(TIME,-2),Effect(PROGRESS,+12),Effect(MORALE,+10),Effect(SAFETY,-3)],
                "消息传回地球，全球震动。国际同行对样本展开联合分析。"),
         Choice("先内部反复验证，确认后再公布",
                [Effect(TIME,-5),Effect(PROGRESS,+8),Effect(MATERIALS,-6)],
                "经过严苛的验证流程，结果依然指向有机起源。公布时更有底气。"),
         Choice("暂时保密，避免引发误报和舆论风险",
                [Effect(TIME,-1),Effect(PROGRESS,+3),Effect(MORALE,-8)],
                "机组对隐瞒发现感到不安。但科学的严谨性要求他们再等等。")],False))

    EVENTS.append(Event("e12",60,85,"攻坚","温室工程","🌱",
        "要实现基地自给自足，必须建立受控生态生命保障系统。"
        "温室工程是整个任务的关键节点——它将决定人类能否在火星上持续生存。",
        [Choice("建造全功能温室，种植多种作物",
                [Effect(TIME,-6),Effect(MATERIALS,-20),Effect(ENERGY,-15),Effect(PROGRESS,+18),Effect(MORALE,+10)],
                "第一株绿色植物在火星土壤中发芽。这是人类文明的又一粒种子。"),
         Choice("先建小型实验温室，验证技术可行性",
                [Effect(TIME,-4),Effect(MATERIALS,-10),Effect(ENERGY,-8),Effect(PROGRESS,+10)],
                "小型温室运行良好，为后续扩建积累了宝贵经验。"),
         Choice("引入藻类生物反应器，高效产氧产食",
                [Effect(TIME,-5),Effect(MATERIALS,-14),Effect(ENERGY,-12),Effect(PROGRESS,+15)],
                "藻类反应器效率惊人，氧气和蛋白质产出超出预期。")],False))

    EVENTS.append(Event("e13",60,85,"攻坚","漫游车远征","🚙",
        "为了勘探基地周边50公里范围内的资源分布，需要组织一次远程漫游车远征。"
        "这是对机组体力、设备可靠性和导航技术的综合考验。",
        [Choice("双人小队，轻装快速侦察",
                [Effect(TIME,-4),Effect(ENERGY,-12),Effect(MATERIALS,+15),Effect(SAFETY,-8),Effect(PROGRESS,+8)],
                "远征队带回了详细的资源分布图，发现了一处高浓度矿物露头。"),
         Choice("四人完整科考队，全面采样",
                [Effect(TIME,-7),Effect(ENERGY,-20),Effect(MATERIALS,+25),Effect(SAFETY,-12),Effect(PROGRESS,+14)],
                "海量的样本和数据让基地的资源储备翻了一番，但代价是巨大的体力消耗。"),
         Choice("使用自主漫游无人机，零人员风险",
                [Effect(TIME,-3),Effect(MATERIALS,-10),Effect(ENERGY,-15),Effect(PROGRESS,+6)],
                "无人机高效完成了测绘，虽然深度采样不足，但保证了人员安全。")],False))

    EVENTS.append(Event("e14",65,88,"攻坚","反应堆维护","⚛️",
        "小型核反应堆的运行时间已接近设计寿命的一半，需要进行一次深度维护。"
        "这涉及高辐射环境作业，必须谨慎安排。",
        [Choice("由受过专项训练的工程师进行手动维护",
                [Effect(TIME,-3),Effect(SAFETY,-15),Effect(ENERGY,+20),Effect(PROGRESS,+6),Effect(MORALE,-5)],
                "维护完美完成，反应堆效率恢复到出厂水平。工程师承受了一定辐射剂量。"),
         Choice("使用遥控机器人进行维护",
                [Effect(TIME,-5),Effect(MATERIALS,-12),Effect(ENERGY,+15),Effect(SAFETY,+5)],
                "虽然耗时更长，但人员零接触辐射。安全性是长征的底线。"),
         Choice("推迟维护，等任务后期再说",
                [Effect(TIME,-1),Effect(SAFETY,-18),Effect(ENERGY,-8)],
                "反应堆开始出现效率衰减。隐患被推迟了，但没有消失。")],False))

    EVENTS.append(Event("e15",82,96,"星辰","自给自足验证","🏗️",
        "地球评估委员会要求基地进行为期两周的\"完全自给自足验证\"："
        "切断所有来自地球的补给模拟，测试基地能否独立维持运转。",
        [Choice("严格执行标准流程，逐项验证所有子系统",
                [Effect(TIME,-5),Effect(ENERGY,-12),Effect(MATERIALS,-8),Effect(SAFETY,+8),Effect(PROGRESS,+12)],
                "所有系统通过验证。基地可以在不依赖地球的情况下持续运行。"),
         Choice("简化流程，重点验证核心生存系统",
                [Effect(TIME,-3),Effect(ENERGY,-6),Effect(PROGRESS,+8),Effect(SAFETY,-5)],
                "核心系统验证通过，但部分子系统的数据不够完整。"),
         Choice("边验证边优化，发现问题立即改进",
                [Effect(TIME,-6),Effect(MATERIALS,-10),Effect(ENERGY,-10),Effect(PROGRESS,+15),Effect(MORALE,+5)],
                "验证中发现并修复了3个潜在问题。基地比验证前更强大了。")],False))

    EVENTS.append(Event("e16",90,100,"星辰","向地球报告","📡",
        "自给自足验证通过，\"瑞金\"基地正式成为人类在火星的第一个永久据点。"
        "现在，你需要向地球提交最终任务报告。这份报告将写入人类史册。",
        [Choice("如实汇报所有数据，包括失误和教训",
                [Effect(PROGRESS,+8),Effect(MORALE,+8)],
                "地球指挥部高度评价了报告的坦诚与专业。失误是未来成功的基石。"),
         Choice("重点突出成就，弱化困难",
                [Effect(PROGRESS,+6),Effect(MORALE,+3),Effect(SAFETY,-3)],
                "报告赢得了一片赞誉，但有经验的人看出了其中的回避。"),
         Choice("以\"长征精神\"为主线，讲述团队的故事",
                [Effect(PROGRESS,+7),Effect(MORALE,+15)],
                "报告感动了无数人。从瑞金到火星，长征的火种永不熄灭。")],False))

def init_game():
    G.sol = 1
    G.maxSol = 180
    G.res = [180, 70, 60, 6, 80, 85, 0]
    G.phaseName = "星火"
    G.ended = False
    G.log = []
    G.usedEvents = []
    G.currentEventId = ""
    G.log.append("任务启动：长征计划 · 火星永久基地建立")

def phase_for_progress(p):
    if p < 25: return "星火"
    if p < 50: return "砥砺"
    if p < 75: return "转折"
    if p < 90: return "攻坚"
    return "星辰"

def pick_next_event():
    p = G.res[PROGRESS]
    candidates = []
    for e in EVENTS:
        if p >= e.minProgress and p <= e.maxProgress:
            if e.id not in G.usedEvents or e.repeatable:
                candidates.append(e)
    if not candidates:
        for e in EVENTS:
            if e.id not in G.usedEvents:
                candidates.append(e)
    if not candidates:
        return None
    return random.choice(candidates)

def apply_effects(choice):
    for ef in choice.effects:
        maxv = 6 if ef.res == PERSONNEL else 100
        G.res[ef.res] = clamp(G.res[ef.res] + ef.delta, 0, maxv)
        if ef.res == TIME:
            G.sol += (-ef.delta)
            G.res[TIME] = max(0, G.maxSol - G.sol + 1)
    return choice.result

def check_ending():
    if G.res[PROGRESS] >= 100:
        G.ended = True
        G.endingWin = True
        G.endingTitle = "任务达成 · 星火燎原"
        G.endingEmoji = "🌠"
        G.endingText = (
            f"在第 {G.sol} 个火星日，\"瑞金\"基地达成全部建设目标。"
            "人类在火星上拥有了第一个可以自给自足的永久据点。\n\n"
            "从1934年的瑞金出发，到2047年的火星\"瑞金\"——两万五千里的足迹，"
            "最终延伸到了星辰大海。长征从未结束，它只是换了一种方式继续。\n\n"
            "你的名字将被刻在火星的岩石上，与那些红米饭、南瓜汤的故事一起，"
            "成为人类不屈精神的永恒注脚。")
        return True
    if G.res[TIME] <= 0:
        G.ended = True
        G.endingWin = False
        G.endingTitle = "任务失败 · 窗口关闭"
        G.endingEmoji = "⌛"
        G.endingText = (
            "地火合相时刻到来，180个火星日的窗口关闭了。基地尚未达到自给自足标准，"
            "与地球的通讯即将中断12周。在没有后援的火星上，机组面临着严酷的生存考验。\n\n"
            "长征路上，时间从来都是最无情的敌人。")
        return True
    if G.res[PERSONNEL] <= 0:
        G.ended = True
        G.endingWin = False
        G.endingTitle = "任务失败 · 牺牲"
        G.endingEmoji = "🕯️"
        G.endingText = (
            "最后一名机组人员倒下了。\"瑞金\"基地成为了火星上一座静默的纪念碑。\n\n"
            "他们的名字将被永远铭记——正如长征路上那些没有走到终点的战士。"
            "他们的牺牲，为后来者照亮了道路。")
        return True
    if G.res[SAFETY] <= 0:
        G.ended = True
        G.endingWin = False
        G.endingTitle = "任务失败 · 基地失守"
        G.endingEmoji = "💥"
        G.endingText = (
            "一场灾难性的系统故障击穿了基地的安全防线。居住舱失压，"
            "机组在最后时刻启动了救生程序。\n\n"
            "长征从来不是坦途。失败不是终点，而是下一次出发的起点。")
        return True
    if G.res[MORALE] <= 0:
        G.ended = True
        G.endingWin = False
        G.endingTitle = "任务失败 · 精神崩溃"
        G.endingEmoji = "😔"
        G.endingText = (
            "持续的高压、孤独和挫折击垮了机组的精神防线。他们请求紧急撤离。\n\n"
            "长征不仅是身体的远征，更是精神的远征。没有坚定的信念，"
            "再先进的技术也无法让人在绝境中坚持下去。")
        return True
    return False

def out(k, v):
    print(f"FIELD {k} {v}", flush=True)

def emit_event(e):
    G.currentEventId = e.id
    G.currentChoiceCount = len(e.choices)
    out("sol", str(G.sol))
    out("phase", e.phase)
    out("progress", str(G.res[PROGRESS]))
    out("time", str(G.res[TIME]))
    out("energy", str(G.res[ENERGY]))
    out("materials", str(G.res[MATERIALS]))
    out("personnel", str(G.res[PERSONNEL]))
    out("morale", str(G.res[MORALE]))
    out("safety", str(G.res[SAFETY]))
    out("title", e.title)
    out("image", e.image)
    out("narrative", e.narrative)
    for i, c in enumerate(e.choices):
        cost = " ".join(f"{res_icon(ef.res)}{'+' if ef.delta > 0 else ''}{ef.delta}" for ef in c.effects)
        print(f"CHOICE {i} {c.text}", flush=True)
        print(f"COST {i} {cost}", flush=True)
    print("END", flush=True)

def emit_ending():
    out("sol", str(G.sol))
    out("progress", str(G.res[PROGRESS]))
    out("time", str(G.res[TIME]))
    out("energy", str(G.res[ENERGY]))
    out("materials", str(G.res[MATERIALS]))
    out("personnel", str(G.res[PERSONNEL]))
    out("morale", str(G.res[MORALE]))
    out("safety", str(G.res[SAFETY]))
    out("ending_title", G.endingTitle)
    out("ending_emoji", G.endingEmoji)
    out("ending_text", G.endingText)
    out("ending_win", "1" if G.endingWin else "0")
    print("END", flush=True)

def emit_result(e, idx):
    out("result", e.choices[idx].result)
    print("END", flush=True)

def main():
    random.seed()
    init_events()
    for line in sys.stdin:
        line = line.rstrip("\r\n")
        if not line:
            continue
        if line == "START":
            init_game()
            e = pick_next_event()
            if e:
                G.usedEvents.append(e.id)
                emit_event(e)
            else:
                emit_ending()
        elif line.startswith("CHOICE "):
            idx = int(line[7:])
            cur = None
            for e in EVENTS:
                if e.id == G.currentEventId:
                    cur = e
                    break
            if cur is None or idx < 0 or idx >= len(cur.choices):
                print("ERROR invalid choice", flush=True)
                print("END", flush=True)
                continue
            result = apply_effects(cur.choices[idx])
            G.log.append(f"[SOL {G.sol}] {cur.title} → {cur.choices[idx].text}")
            G.sol += 1
            G.res[TIME] = max(0, G.maxSol - G.sol + 1)
            G.phaseName = phase_for_progress(G.res[PROGRESS])
            emit_result(cur, idx)
        elif line == "NEXT":
            if check_ending():
                emit_ending()
            else:
                e = pick_next_event()
                if e:
                    G.usedEvents.append(e.id)
                    emit_event(e)
                else:
                    if G.res[PROGRESS] >= 100:
                        G.res[PROGRESS] = 100
                    check_ending()
                    emit_ending()
        elif line == "QUIT":
            break

if __name__ == "__main__":
    main()
