// =====================================================================
//  长征 · 从烽火到星辰 —— Java Swing 前端
//  启动 C++ 引擎进程 (engine.exe / engine)，通过 stdin/stdout 通信
//  编译：javac Game.java
//  运行：java Game
// =====================================================================
import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Game {
    // ---------- 颜色主题 (长征：烽火红 + 星辰金) ----------
    static final Color BG_DEEP     = new Color(18, 14, 22);
    static final Color BG_PANEL    = new Color(30, 22, 38);
    static final Color BG_PANEL_2  = new Color(42, 32, 52);
    static final Color RED         = new Color(200, 16, 46);
    static final Color RED_LIGHT   = new Color(230, 60, 70);
    static final Color GOLD        = new Color(212, 175, 55);
    static final Color GOLD_LIGHT  = new Color(240, 200, 90);
    static final Color TEXT        = new Color(240, 230, 210);
    static final Color TEXT_DIM    = new Color(160, 145, 130);
    static final Color BORDER      = new Color(120, 80, 60);

    // 资源条颜色
    static final Color C_TIME      = new Color(200, 200, 200);
    static final Color C_ENERGY    = new Color(79, 195, 247);
    static final Color C_MATERIALS = new Color(255, 213, 79);
    static final Color C_PERSONNEL = new Color(186, 104, 200);
    static final Color C_MORALE    = new Color(239, 83, 80);
    static final Color C_SAFETY    = new Color(129, 199, 132);

    // ---------- 组件 ----------
    JFrame frame;
    JLabel solLabel, phaseLabel;
    JProgressBar progressBar;
    JLabel progressText;
    JLabel[] resValLabels = new JLabel[6];
    JProgressBar[] resBars = new JProgressBar[6];
    JPanel resCards;
    JLabel eventTitle, eventImage;
    JTextArea narrativeText;
    JPanel choicesPanel;
    DefaultListModel<String> logModel;
    JList<String> logList;

    // ---------- 引擎进程 ----------
    Process engine;
    BufferedReader engineIn;
    PrintWriter engineOut;

    // ---------- 状态 ----------
    String currentNarrative = "";
    boolean awaitingResult = false;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new Game().buildAndShow());
    }

    void buildAndShow() {
        try { UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName()); }
        catch (Exception e) {}

        frame = new JFrame("长征 · 从烽火到星辰");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.getContentPane().setBackground(BG_DEEP);
        frame.setLayout(new BorderLayout(0, 0));
        frame.setMinimumSize(new Dimension(1000, 720));

        // 启动引擎
        if (!startEngine()) {
            JOptionPane.showMessageDialog(null,
                "无法启动游戏引擎。\n请确认已编译 engine.cpp 生成 engine.exe（Windows）或 engine。\n\n编译命令：g++ -std=c++17 engine.cpp -o engine",
                "引擎启动失败", JOptionPane.ERROR_MESSAGE);
            System.exit(1);
        }

        buildUI();
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);

        // 开始游戏
        sendCommand("START");
    }

    // ---------- 获取程序所在目录（jar/exe 所在位置） ----------
    String getAppDir() {
        try {
            java.io.File f = new java.io.File(Game.class.getProtectionDomain()
                .getCodeSource().getLocation().toURI());
            String dir = f.getParent();
            if (dir == null) dir = ".";
            return dir;
        } catch (Exception e) {
            return ".";
        }
    }

    // ---------- 启动引擎：优先 C++ 编译版，回退到 Python 版 ----------
    boolean startEngine() {
        String os = System.getProperty("os.name").toLowerCase();
        String appDir = getAppDir();
        String sep = java.io.File.separator;
        String engineExe = appDir + sep + (os.contains("win") ? "engine.exe" : "engine");
        String enginePy = appDir + sep + "engine.py";
        // 1) 尝试已编译的 C++ 引擎
        if (tryStart(engineExe)) return true;
        // 2) 回退到 Python 版引擎
        String py = os.contains("win") ? "python" : "python3";
        if (tryStart(py, enginePy)) return true;
        return false;
    }

    boolean tryStart(String... cmd) {
        try {
            ProcessBuilder pb = new ProcessBuilder(cmd);
            pb.redirectErrorStream(true);
            engine = pb.start();
            engineIn = new BufferedReader(new InputStreamReader(engine.getInputStream(), "UTF-8"));
            engineOut = new PrintWriter(new OutputStreamWriter(engine.getOutputStream(), "UTF-8"), true);
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    // ---------- 构建界面 ----------
    void buildUI() {
        // ===== 顶部 HUD =====
        JPanel hud = new JPanel(new BorderLayout(16, 0));
        hud.setBackground(BG_PANEL);
        hud.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(0, 0, 1, 0, BORDER),
            BorderFactory.createEmptyBorder(12, 24, 12, 24)));

        JPanel hudLeft = new JPanel(new GridLayout(2, 1, 0, 2));
        hudLeft.setOpaque(false);
        JLabel baseName = new JLabel("长征计划 · 火星永久基地「瑞金」");
        baseName.setForeground(TEXT);
        baseName.setFont(baseName.getFont().deriveFont(Font.BOLD, 16f));
        solLabel = new JLabel("SOL 1 / 180");
        solLabel.setForeground(GOLD);
        solLabel.setFont(solLabel.getFont().deriveFont(Font.BOLD, 13f));
        hudLeft.add(baseName);
        hudLeft.add(solLabel);
        hud.add(hudLeft, BorderLayout.WEST);

        JPanel progPanel = new JPanel(new BorderLayout(8, 0));
        progPanel.setOpaque(false);
        JLabel progLabel = new JLabel("任务进度");
        progLabel.setForeground(TEXT_DIM);
        progLabel.setFont(progLabel.getFont().deriveFont(12f));
        progressBar = new JProgressBar(0, 100);
        progressBar.setValue(0);
        progressBar.setStringPainted(false);
        progressBar.setBackground(new Color(40, 30, 20));
        progressBar.setForeground(new Color(120, 180, 120));
        progressBar.setBorder(BorderFactory.createLineBorder(BORDER));
        progressText = new JLabel("0%");
        progressText.setForeground(new Color(129, 199, 132));
        progressText.setFont(progressText.getFont().deriveFont(Font.BOLD, 14f));
        progPanel.add(progLabel, BorderLayout.WEST);
        progPanel.add(progressBar, BorderLayout.CENTER);
        progPanel.add(progressText, BorderLayout.EAST);
        hud.add(progPanel, BorderLayout.CENTER);

        phaseLabel = new JLabel("星火");
        phaseLabel.setForeground(GOLD);
        phaseLabel.setFont(phaseLabel.getFont().deriveFont(Font.BOLD, 13f));
        phaseLabel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(GOLD),
            BorderFactory.createEmptyBorder(4, 14, 4, 14)));
        hud.add(phaseLabel, BorderLayout.EAST);

        frame.add(hud, BorderLayout.NORTH);

        // ===== 主区域 =====
        JPanel main = new JPanel(new BorderLayout(16, 0));
        main.setBackground(BG_DEEP);
        main.setBorder(BorderFactory.createEmptyBorder(16, 24, 24, 24));

        // 资源面板
        String[] resNames = {"时间", "能源", "物资", "人员", "士气", "安全"};
        String[] resIcons = {"⏳", "⚡", "📦", "👥", "❤️", "🛡"};
        Color[] resColors = {C_TIME, C_ENERGY, C_MATERIALS, C_PERSONNEL, C_MORALE, C_SAFETY};
        resCards = new JPanel(new GridLayout(1, 6, 12, 0));
        resCards.setOpaque(false);
        for (int i = 0; i < 6; i++) {
            JPanel card = new JPanel();
            card.setBackground(BG_PANEL);
            card.setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(BORDER),
                BorderFactory.createEmptyBorder(10, 12, 10, 12)));
            card.setLayout(new BoxLayout(card, BoxLayout.Y_AXIS));

            JPanel row1 = new JPanel(new BorderLayout());
            row1.setOpaque(false);
            JLabel icon = new JLabel(resIcons[i]);
            icon.setFont(icon.getFont().deriveFont(22f));
            JLabel name = new JLabel(resNames[i]);
            name.setForeground(TEXT_DIM);
            name.setFont(name.getFont().deriveFont(12f));
            resValLabels[i] = new JLabel("--");
            resValLabels[i].setForeground(TEXT);
            resValLabels[i].setFont(resValLabels[i].getFont().deriveFont(Font.BOLD, 13f));
            row1.add(icon, BorderLayout.WEST);
            row1.add(name, BorderLayout.CENTER);
            row1.add(resValLabels[i], BorderLayout.EAST);
            card.add(row1);

            resBars[i] = new JProgressBar(0, 100);
            resBars[i].setValue(100);
            resBars[i].setStringPainted(false);
            resBars[i].setBackground(new Color(50, 40, 30));
            resBars[i].setForeground(resColors[i]);
            resBars[i].setBorderPainted(false);
            resBars[i].setPreferredSize(new Dimension(0, 8));
            resBars[i].setMaximumSize(new Dimension(Integer.MAX_VALUE, 8));
            card.add(Box.createVerticalStrut(6));
            card.add(resBars[i]);

            resCards.add(card);
        }
        main.add(resCards, BorderLayout.NORTH);

        // 叙事 + 日志
        JSplitPane split = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        split.setOpaque(false);
        split.setBorder(null);
        split.setDividerLocation(720);
        split.setResizeWeight(1.0);

        // 叙事面板
        JPanel narrative = new JPanel(new BorderLayout(0, 12));
        narrative.setBackground(BG_PANEL);
        narrative.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(BORDER),
            BorderFactory.createEmptyBorder(24, 28, 24, 28)));

        eventTitle = new JLabel("—");
        eventTitle.setForeground(RED_LIGHT);
        eventTitle.setFont(eventTitle.getFont().deriveFont(Font.BOLD, 22f));
        eventTitle.setBorder(BorderFactory.createEmptyBorder(0, 0, 8, 0));
        narrative.add(eventTitle, BorderLayout.NORTH);

        JPanel center = new JPanel(new BorderLayout(0, 12));
        center.setOpaque(false);
        eventImage = new JLabel("🚀", SwingConstants.CENTER);
        eventImage.setFont(eventImage.getFont().deriveFont(56f));
        eventImage.setBorder(BorderFactory.createEmptyBorder(4, 0, 4, 0));
        center.add(eventImage, BorderLayout.NORTH);

        narrativeText = new JTextArea();
        narrativeText.setEditable(false);
        narrativeText.setLineWrap(true);
        narrativeText.setWrapStyleWord(true);
        narrativeText.setBackground(BG_PANEL);
        narrativeText.setForeground(TEXT);
        narrativeText.setFont(narrativeText.getFont().deriveFont(15f));
        narrativeText.setBorder(null);
        JScrollPane narrScroll = new JScrollPane(narrativeText);
        narrScroll.setBorder(null);
        narrScroll.getVerticalScrollBar().setUnitIncrement(16);
        narrScroll.getVerticalScrollBar().setBackground(BG_PANEL);
        center.add(narrScroll, BorderLayout.CENTER);
        narrative.add(center, BorderLayout.CENTER);

        choicesPanel = new JPanel();
        choicesPanel.setLayout(new BoxLayout(choicesPanel, BoxLayout.Y_AXIS));
        choicesPanel.setOpaque(false);
        choicesPanel.setBorder(BorderFactory.createEmptyBorder(12, 0, 0, 0));
        narrative.add(choicesPanel, BorderLayout.SOUTH);

        split.setLeftComponent(narrative);

        // 日志面板
        JPanel logPanel = new JPanel(new BorderLayout());
        logPanel.setBackground(BG_PANEL);
        logPanel.setBorder(BorderFactory.createLineBorder(BORDER));
        JLabel logHeader = new JLabel("  任务日志");
        logHeader.setForeground(GOLD);
        logHeader.setFont(logHeader.getFont().deriveFont(Font.BOLD, 13f));
        logHeader.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(0, 0, 1, 0, BORDER),
            BorderFactory.createEmptyBorder(10, 0, 10, 0)));
        logPanel.add(logHeader, BorderLayout.NORTH);

        logModel = new DefaultListModel<>();
        logList = new JList<>(logModel);
        logList.setBackground(BG_PANEL);
        logList.setForeground(TEXT_DIM);
        logList.setFont(logList.getFont().deriveFont(12f));
        logList.setCellRenderer(new LogRenderer());
        logList.setBorder(BorderFactory.createEmptyBorder(8, 10, 8, 10));
        JScrollPane logScroll = new JScrollPane(logList);
        logScroll.setBorder(null);
        logScroll.getVerticalScrollBar().setBackground(BG_PANEL);
        logPanel.add(logScroll, BorderLayout.CENTER);

        split.setRightComponent(logPanel);
        main.add(split, BorderLayout.CENTER);

        frame.add(main, BorderLayout.CENTER);
    }

    // 日志渲染器
    static class LogRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value,
                int index, boolean sel, boolean focus) {
            JLabel c = (JLabel) super.getListCellRendererComponent(list, value, index, sel, focus);
            c.setBorder(BorderFactory.createEmptyBorder(4, 0, 4, 0));
            String s = value == null ? "" : value.toString();
            if (s.startsWith("[成功]") || s.contains("达成")) {
                c.setForeground(new Color(129, 199, 132));
            } else if (s.startsWith("[失败]") || s.contains("失败") || s.contains("牺牲")) {
                c.setForeground(new Color(239, 83, 80));
            } else {
                c.setForeground(TEXT_DIM);
            }
            return c;
        }
    }

    // ---------- 通信 ----------
    void sendCommand(String cmd) {
        new Thread(() -> {
            try {
                engineOut.println(cmd);
                Map<String, String> fields = new LinkedHashMap<>();
                List<String[]> choices = new ArrayList<>();
                List<String[]> costs = new ArrayList<>();
                String line;
                while ((line = engineIn.readLine()) != null) {
                    if (line.equals("END")) break;
                    if (line.startsWith("FIELD ")) {
                        String rest = line.substring(6);
                        int sp = rest.indexOf(' ');
                        if (sp > 0) fields.put(rest.substring(0, sp), rest.substring(sp + 1));
                    } else if (line.startsWith("CHOICE ")) {
                        String rest = line.substring(7);
                        int sp = rest.indexOf(' ');
                        if (sp > 0) choices.add(new String[]{rest.substring(0, sp), rest.substring(sp + 1)});
                    } else if (line.startsWith("COST ")) {
                        String rest = line.substring(5);
                        int sp = rest.indexOf(' ');
                        if (sp > 0) costs.add(new String[]{rest.substring(0, sp), rest.substring(sp + 1)});
                        else costs.add(new String[]{rest, ""});
                    }
                }
                // 在 EDT 更新 UI
                SwingUtilities.invokeLater(() -> handleResponse(fields, choices, costs));
            } catch (IOException e) {
                e.printStackTrace();
                SwingUtilities.invokeLater(() ->
                    JOptionPane.showMessageDialog(frame, "与引擎通信失败：" + e.getMessage(),
                        "错误", JOptionPane.ERROR_MESSAGE));
            }
        }).start();
    }

    void handleResponse(Map<String, String> fields, List<String[]> choices, List<String[]> costs) {
        // 更新资源
        if (fields.containsKey("sol")) {
            solLabel.setText("SOL " + fields.get("sol") + " / 180");
        }
        if (fields.containsKey("phase")) {
            phaseLabel.setText(fields.get("phase"));
        }
        if (fields.containsKey("progress")) {
            int p = Integer.parseInt(fields.get("progress"));
            progressBar.setValue(p);
            progressText.setText(p + "%");
        }
        String[] resKeys = {"time", "energy", "materials", "personnel", "morale", "safety"};
        String[] resUnits = {"sol", "/100", "/100", "人", "/100", "/100"};
        int[] resMax   = {180, 100, 100, 6, 100, 100};
        for (int i = 0; i < 6; i++) {
            if (fields.containsKey(resKeys[i])) {
                int v = Integer.parseInt(fields.get(resKeys[i]));
                resBars[i].setValue((int) Math.round(100.0 * v / resMax[i]));
                resValLabels[i].setText(v + " " + resUnits[i]);
                // 低值警告
                boolean low = (i < 2 && v < 20) || (i == 3 && v <= 2) || (i >= 4 && v < 25);
                resValLabels[i].setForeground(low ? RED_LIGHT : TEXT);
            }
        }

        // 结局
        if (fields.containsKey("ending_title")) {
            showEnding(fields);
            return;
        }

        // 选择结果描述（过渡）
        if (fields.containsKey("result") && !fields.containsKey("title")) {
            addLog(fields.get("result"));
            // 显示结果后请求下一个事件
            sendCommand("NEXT");
            return;
        }

        // 事件
        if (fields.containsKey("title")) {
            eventTitle.setText(fields.get("title"));
            eventImage.setText(fields.getOrDefault("image", "📡"));
            narrativeText.setText(fields.getOrDefault("narrative", ""));
            narrativeText.setCaretPosition(0);
            renderChoices(choices, costs);
        }
    }

    void renderChoices(List<String[]> choices, List<String[]> costs) {
        choicesPanel.removeAll();
        for (String[] ch : choices) {
            int idx = Integer.parseInt(ch[0]);
            String text = ch[1];
            String cost = "";
            for (String[] c : costs) if (c[0].equals(ch[0])) { cost = c[1]; break; }

            JButton btn = new JButton("<html><div style='padding:4px 2px;color:#000000;'>" + text +
                "</div><div style='font-size:11px;color:" + hex(GOLD) + ";padding-top:4px;'>" + cost + "</div></html>");
            btn.setBackground(new Color(232, 222, 205));
            btn.setForeground(Color.BLACK);
            btn.setFocusPainted(false);
            btn.setHorizontalAlignment(SwingConstants.LEFT);
            btn.setFont(btn.getFont().deriveFont(14f));
            btn.setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createMatteBorder(0, 4, 0, 0, C_ENERGY),
                BorderFactory.createEmptyBorder(10, 14, 10, 14)));
            btn.setCursor(new Cursor(Cursor.HAND_CURSOR));
            btn.setAlignmentX(Component.LEFT_ALIGNMENT);
            btn.setMaximumSize(new Dimension(Integer.MAX_VALUE, btn.getPreferredSize().height));

            btn.addMouseListener(new MouseAdapter() {
                @Override public void mouseEntered(MouseEvent e) {
                    btn.setBackground(new Color(245, 235, 215));
                    btn.setBorder(BorderFactory.createCompoundBorder(
                        BorderFactory.createMatteBorder(0, 4, 0, 0, RED),
                        BorderFactory.createEmptyBorder(10, 14, 10, 14)));
                }
                @Override public void mouseExited(MouseEvent e) {
                    btn.setBackground(new Color(232, 222, 205));
                    btn.setBorder(BorderFactory.createCompoundBorder(
                        BorderFactory.createMatteBorder(0, 4, 0, 0, C_ENERGY),
                        BorderFactory.createEmptyBorder(10, 14, 10, 14)));
                }
            });
            btn.addActionListener(e -> onChoice(idx));

            choicesPanel.add(btn);
            choicesPanel.add(Box.createVerticalStrut(10));
        }
        choicesPanel.revalidate();
        choicesPanel.repaint();
    }

    void onChoice(int idx) {
        // 禁用所有按钮
        for (Component c : choicesPanel.getComponents()) {
            if (c instanceof JButton) c.setEnabled(false);
        }
        sendCommand("CHOICE " + idx);
    }

    void addLog(String msg) {
        logModel.addElement(msg);
        logList.ensureIndexIsVisible(logModel.getSize() - 1);
    }

    void showEnding(Map<String, String> f) {
        boolean win = "1".equals(f.get("ending_win"));
        String title = f.get("ending_title");
        String emoji = f.getOrDefault("ending_emoji", "🛰");
        String text = f.getOrDefault("ending_text", "");

        eventTitle.setText(title);
        eventImage.setText(emoji);
        narrativeText.setText(text);
        choicesPanel.removeAll();

        JButton restart = new JButton("重新开始任务");
        restart.setBackground(new Color(232, 222, 205));
        restart.setForeground(Color.BLACK);
        restart.setFocusPainted(false);
        restart.setFont(restart.getFont().deriveFont(Font.BOLD, 15f));
        restart.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(0, 4, 0, 0, RED),
            BorderFactory.createEmptyBorder(12, 40, 12, 40)));
        restart.setAlignmentX(Component.CENTER_ALIGNMENT);
        restart.setCursor(new Cursor(Cursor.HAND_CURSOR));
        restart.addActionListener(e -> {
            // 重启引擎进程
            try { engine.destroyForcibly(); } catch (Exception ex) {}
            startEngine();
            logModel.clear();
            sendCommand("START");
        });
        choicesPanel.add(restart);
        choicesPanel.revalidate();
        choicesPanel.repaint();

        addLog((win ? "[成功] " : "[失败] ") + title);
    }

    String hex(Color c) {
        return String.format("#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue());
    }
}
