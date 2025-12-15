#include "calculator.h"
#include "webview_manager.h"
#include "common.h"
#include "logger.h"
#include <windows.h>
#include <shlobj.h>    // 包含CSIDL_APPDATA和SHGetFolderPathW定义
#include <commctrl.h>  // 包含列表视图控件相关定义
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
// #include <codecvt> // Removed deprecated header

// 全局变量声明（在gui_main.cpp中定义）
extern HWND g_hListView;
extern HWND g_hEdit;
extern bool g_calculatorMode;
extern std::vector<CalculationRecord> g_calculationHistory;

// 自定义公式全局变量
std::vector<CustomFormula> g_customFormulas;
static std::wstring g_pendingComment;
static std::wstring g_lastEvaluatedExpr;

// 前向声明
extern void UpdateWindowTitle();

/**
 * @brief 读取文件内容到wstring (UTF-8)
 */
static std::wstring ReadFileToString(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return L"";

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart == 0)
    {
        CloseHandle(hFile);
        return L"";
    }

    // 读取全部内容
    DWORD size = (DWORD)fileSize.QuadPart;
    std::vector<char> buffer(size);
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer.data(), size, &bytesRead, NULL) || bytesRead == 0)
    {
        CloseHandle(hFile);
        return L"";
    }
    CloseHandle(hFile);

    // 处理BOM (UTF-8 BOM: EF BB BF)
    char* pData = buffer.data();
    if (bytesRead >= 3 && (unsigned char)pData[0] == 0xEF && (unsigned char)pData[1] == 0xBB && (unsigned char)pData[2] == 0xBF)
    {
        pData += 3;
        bytesRead -= 3;
    }

    if (bytesRead == 0) return L"";

    // 转换为wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, pData, bytesRead, NULL, 0);
    if (wlen <= 0) return L"";

    std::wstring result(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, pData, bytesRead, &result[0], wlen);

    return result;
}

/**
 * @brief 将wstring写入文件 (UTF-8)
 */
static bool WriteStringToFile(const std::wstring& filePath, const std::wstring& content)
{
    // 转换为UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) return false;

    std::vector<char> buffer(len - 1); // -1 是为了去掉结尾的 null terminator，如果 content 不包含 \0 则需要调整，这里 -1 是因为 WC2MB 返回包含 null 的长度
    // 实际上 WideCharToMultiByte 如果传入 -1，返回长度包含 null。写入文件不需要 null。
    
    // 重新计算不含null的长度
    len = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), (int)content.length(), NULL, 0, NULL, NULL);
    if (len <= 0) return false;
    
    buffer.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), (int)content.length(), buffer.data(), len, NULL, NULL);

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten = 0;
    bool success = WriteFile(hFile, buffer.data(), len, &bytesWritten, NULL);
    CloseHandle(hFile);

    return success;
}

/**
 * @brief 加载自定义公式
 */
void LoadCustomFormulas()
{
    g_customFormulas.clear();

    // 定义默认公式列表
    struct DefaultFormula {
        std::wstring name;
        std::wstring expr;
        std::wstring desc;
    };

    std::vector<DefaultFormula> defaults = {
        {L"circleArea", L"function(r) { return Math.PI * r * r; }", L"计算圆的面积 (r: 半径)"},
        {L"rectArea", L"function(w, h) { return w * h; }", L"计算矩形的面积 (w: 宽, h: 高)"},
        {L"triangleArea", L"function(b, h) { return 0.5 * b * h; }", L"计算三角形的面积 (b: 底, h: 高)"},
        {L"bmi", L"function(weight, height) { return weight / ((height/100) * (height/100)); }", L"计算BMI指数 (weight: kg, height: cm)"},
        {L"hypotenuse", L"function(a, b) { return Math.sqrt(a*a + b*b); }", L"计算直角三角形斜边 (a, b: 直角边)"},
        {L"c2f", L"function(c) { return (c * 9/5) + 32; }", L"摄氏度转华氏度 (c: 摄氏度)"},
        {L"f2c", L"function(f) { return (f - 32) * 5/9; }", L"华氏度转摄氏度 (f: 华氏度)"},
        {L"discount", L"function(price, rate) { return price * (1 - rate/100); }", L"计算折后价格 (price: 原价, rate: 折扣率%)"},
        {L"sphereVol", L"function(r) { return (4/3) * Math.PI * Math.pow(r, 3); }", L"计算球体体积 (r: 半径)"},
        {L"cylinderVol", L"function(r, h) { return Math.PI * r * r * h; }", L"计算圆柱体体积 (r: 底半径, h: 高)"},
        {L"random", L"function(min, max) { return Math.floor(Math.random() * (max - min + 1)) + min; }", L"生成随机整数 (min: 最小值, max: 最大值)"},
        {L"cubeVol", L"function(a) { return Math.pow(a, 3); }", L"计算立方体体积 (a: 边长)"},
        {L"sphereArea", L"function(r) { return 4 * Math.PI * r * r; }", L"计算球体表面积 (r: 半径)"},
        {L"cylinderArea", L"function(r, h) { return 2 * Math.PI * r * (r + h); }", L"计算圆柱体表面积 (r: 底半径, h: 高)"},
        {L"coneVol", L"function(r, h) { return (1.0/3.0) * Math.PI * r * r * h; }", L"计算圆锥体体积 (r: 底半径, h: 高)"},
        {L"trapezoidArea", L"function(a, b, h) { return (a + b) * h / 2; }", L"计算梯形面积 (a: 上底, b: 下底, h: 高)"},
        {L"heronArea", L"function(a, b, c) { return 0.25 * Math.sqrt((a+b+c)*(a+b-c)*(a+c-b)*(b+c-a)); }", L"海伦公式计算三角形面积 (a, b, c: 三边长)"},
        {L"deg2rad", L"function(deg) { return deg * Math.PI / 180; }", L"角度转弧度 (deg: 角度)"},
        {L"rad2deg", L"function(rad) { return rad * 180 / Math.PI; }", L"弧度转角度 (rad: 弧度)"},
        {L"distance", L"function(x1, y1, x2, y2) { return Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2)); }", L"两点间距离 (x1, y1, x2, y2: 坐标)"},
        {L"logBase", L"function(x, base) { return Math.log(x) / Math.log(base); }", L"计算任意底数的对数 (x: 真数, base: 底数)"},
        {L"factorial", L"function(n) { if (n <= 1) return 1; return n * factorial(n - 1); }", L"计算阶乘 (n: 整数)"}
    };

    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    path = path.substr(0, path.find_last_of(L"\\"));
    std::wstring dirPath = path + L"\\data\\formulas";

    CreateDirectoryW((path + L"\\data").c_str(), NULL);
    CreateDirectoryW(dirPath.c_str(), NULL);

    WIN32_FIND_DATAW fd;
    std::wstring pattern = dirPath + L"\\*.js";
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::wstring fileName = fd.cFileName;
                size_t dot = fileName.find_last_of(L'.');
                std::wstring base = (dot != std::wstring::npos) ? fileName.substr(0, dot) : fileName;
                std::wstring fullPath = dirPath + L"\\" + fileName;

                std::wstring content = ReadFileToString(fullPath);

                CustomFormula f;
                f.name = base;
                
                // 尝试从文件内容解析描述 (// Description)
                std::wstring fileDesc = L"";
                std::wstring fileExpr = content;
                
                // 移除BOM和空白
                size_t firstContent = fileExpr.find_first_not_of(L" \t\r\n\uFEFF");
                if (firstContent != std::wstring::npos)
                {
                    if (fileExpr.substr(firstContent, 2) == L"//")
                    {
                        size_t eol = fileExpr.find(L'\n', firstContent);
                        if (eol != std::wstring::npos)
                        {
                            fileDesc = fileExpr.substr(firstContent + 2, eol - (firstContent + 2));
                            // Trim description
                            size_t dStart = fileDesc.find_first_not_of(L" \t\r");
                            size_t dEnd = fileDesc.find_last_not_of(L" \t\r");
                            if (dStart != std::wstring::npos)
                                fileDesc = fileDesc.substr(dStart, dEnd - dStart + 1);
                            else
                                fileDesc = L"";
                                
                            fileExpr = fileExpr.substr(eol + 1);
                        }
                    }
                }
                
                // Trim expression
                size_t eStart = fileExpr.find_first_not_of(L" \t\r\n");
                if (eStart != std::wstring::npos) fileExpr = fileExpr.substr(eStart);

                f.expression = fileExpr;
                f.description = fileDesc;
                
                // 尝试从默认列表中匹配描述，并自动修复旧格式公式或补充描述
                bool needsRewrite = false;
                
                for (const auto& def : defaults)
                {
                    if (def.name == base)
                    {
                        // 如果文件里没描述，使用默认描述
                        if (f.description.empty())
                        {
                            f.description = def.desc;
                            needsRewrite = true; // 需要回写描述到文件
                        }
                        
                        // 检查是否需要更新旧格式公式（非函数格式 -> 函数格式）
                        bool needsUpdateExpr = false;
                        if (fileExpr.find(L"function") == std::wstring::npos && def.expr.find(L"function") != std::wstring::npos)
                        {
                            needsUpdateExpr = true;
                        }
                        else if (content.find(L"\uFEFF") != std::wstring::npos && content.find(L"\uFEFF") > 0) 
                        {
                            needsUpdateExpr = true; // 修复非正常的BOM
                        }
                        
                        if (needsUpdateExpr)
                        {
                            f.expression = def.expr;
                            needsRewrite = true;
                        }
                        
                        break;
                    }
                }
                
                // 如果需要，回写文件（包含描述注释）
                if (needsRewrite)
                {
                    std::wstring newContent = L"// " + f.description + L"\n" + f.expression;
                    WriteStringToFile(fullPath, newContent);
                }

                g_customFormulas.push_back(f);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    // 检查并添加缺失的默认公式
    for (const auto& def : defaults)
    {
        bool exists = false;
        for (const auto& f : g_customFormulas)
        {
            if (f.name == def.name)
            {
                exists = true;
                break;
            }
        }
        
        if (!exists)
        {
            AddCustomFormula(def.name, def.expr, def.desc);
        }
    }
    
}

/**
 * @brief 保存自定义公式
 */
void SaveCustomFormulas()
{
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    path = path.substr(0, path.find_last_of(L"\\"));
    std::wstring dirPath = path + L"\\data\\formulas";
    CreateDirectoryW((path + L"\\data").c_str(), NULL);
    CreateDirectoryW(dirPath.c_str(), NULL);

    for (const auto& f : g_customFormulas)
    {
        std::wstring fullPath = dirPath + L"\\" + f.name + L".js";
        std::wstring content = L"// " + f.description + L"\n" + f.expression;
        WriteStringToFile(fullPath, content);
    }
}

/**
 * @brief 添加自定义公式
 */
void AddCustomFormula(const std::wstring& name, const std::wstring& expression, const std::wstring& description)
{
    CustomFormula f;
    f.name = name;
    f.expression = expression;
    f.description = description;
    g_customFormulas.push_back(f);

    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    path = path.substr(0, path.find_last_of(L"\\"));
    std::wstring dirPath = path + L"\\data\\formulas";
    CreateDirectoryW((path + L"\\data").c_str(), NULL);
    CreateDirectoryW(dirPath.c_str(), NULL);
    std::wstring fullPath = dirPath + L"\\" + name + L".js";
    std::wstring content = L"// " + description + L"\n" + expression;
    WriteStringToFile(fullPath, content);
}

/**
 * @brief 删除自定义公式
 */
void DeleteCustomFormula(int index)
{
    if (index < 0 || index >= (int)g_customFormulas.size()) return;
    std::wstring name = g_customFormulas[index].name;

    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    path = path.substr(0, path.find_last_of(L"\\"));
    std::wstring fullPath = path + L"\\data\\formulas\\" + name + L".js";
    _wremove(fullPath.c_str());
    g_customFormulas.erase(g_customFormulas.begin() + index);
}

/**
 * @brief 删除自定义公式 (按名称)
 */
void DeleteCustomFormulaByName(const std::wstring& name)
{
    for (auto it = g_customFormulas.begin(); it != g_customFormulas.end(); ++it)
    {
        if (it->name == name)
        {
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring path = exePath;
            path = path.substr(0, path.find_last_of(L"\\"));
            std::wstring fullPath = path + L"\\data\\formulas\\" + name + L".js";
            _wremove(fullPath.c_str());
            g_customFormulas.erase(it);
            return;
        }
    }
}

/**
 * @brief 删除计算历史记录
 */
void DeleteCalculationHistory(int index)
{
    if (index >= 0 && index < (int)g_calculationHistory.size())
    {
        g_calculationHistory.erase(g_calculationHistory.begin() + index);
        LogToFile("DeleteCalculationHistory: 删除了计算历史记录");
    }
}

/**
 * @brief 编辑自定义公式
 */
void EditCustomFormula(const std::wstring& oldName, const std::wstring& newName, const std::wstring& newExpression, const std::wstring& newDescription)
{
    for (auto& f : g_customFormulas)
    {
        if (f.name == oldName)
        {
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring path = exePath;
            path = path.substr(0, path.find_last_of(L"\\"));
            std::wstring dirPath = path + L"\\data\\formulas";
            CreateDirectoryW((path + L"\\data").c_str(), NULL);
            CreateDirectoryW(dirPath.c_str(), NULL);

            std::wstring oldPath = dirPath + L"\\" + oldName + L".js";
            std::wstring newPath = dirPath + L"\\" + newName + L".js";
            if (_wcsicmp(oldName.c_str(), newName.c_str()) != 0)
            {
                _wremove(newPath.c_str());
                MoveFileW(oldPath.c_str(), newPath.c_str());
            }
            std::wstring content = L"// " + newDescription + L"\n" + newExpression;
            WriteStringToFile(newPath, content);
            f.name = newName;
            f.expression = newExpression;
            f.description = newDescription;
            return;
        }
    }
}

/**
 * @brief 表达式解析辅助函数 - 解析数字
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的数字
 */
double parseNumber(const std::wstring& expr, size_t& pos)
{
    std::wstring numStr;
    while (pos < expr.length() && (iswdigit(expr[pos]) || expr[pos] == L'.'))
    {
        numStr += expr[pos];
        pos++;
    }
    return numStr.empty() ? 0.0 : _wtof(numStr.c_str());
}

double parseFactor(const std::wstring& expr, size_t& pos)
{
    if (pos >= expr.length()) return 0.0;
    if (expr[pos] == L'+') { pos++; return parseFactor(expr, pos); }
    if (expr[pos] == L'-') { pos++; return -parseFactor(expr, pos); }
    if (expr[pos] == L'(') { pos++; double v = parseExpression(expr, pos); if (pos < expr.length() && expr[pos] == L')') pos++; return v; }
    if (iswalpha(expr[pos])) {
        std::wstring id;
        while (pos < expr.length() && (iswalpha(expr[pos]) || expr[pos] == L'_')) { id += expr[pos]; pos++; }
        if (pos < expr.length() && expr[pos] == L'(') {
            pos++;
            double arg = parseExpression(expr, pos);
            if (pos < expr.length() && expr[pos] == L')') pos++;
            if (id == L"sin") return std::sin(arg);
            if (id == L"cos") return std::cos(arg);
            if (id == L"tan") return std::tan(arg);
            if (id == L"sqrt") return std::sqrt(arg);
            if (id == L"abs") return std::fabs(arg);
            return 0.0;
        }

        // Handle function call with arguments
        if (pos < expr.length() && expr[pos] == L'(') {
            pos++; // eat '('
            std::vector<double> args;
            if (pos < expr.length() && expr[pos] != L')') {
                args.push_back(parseExpression(expr, pos));
                while (pos < expr.length() && expr[pos] == L',') {
                    pos++; // eat ','
                    args.push_back(parseExpression(expr, pos));
                }
            }
            if (pos < expr.length() && expr[pos] == L')') pos++; // eat ')'
            
            if (args.empty()) return 0.0; // or handle empty args

            double arg1 = args[0];
            
            if (id == L"sin") return std::sin(arg1);
            if (id == L"cos") return std::cos(arg1);
            if (id == L"tan") return std::tan(arg1);
            if (id == L"sqrt") return std::sqrt(arg1);
            if (id == L"abs") return std::fabs(arg1);
            
            // Check custom formulas
            for (const auto& formula : g_customFormulas)
            {
                if (id == formula.name)
                {
                    std::wstring fExpr = formula.expression;
                    
                    // Replace variables x, y, z...
                    auto replaceVar = [&](wchar_t varChar, double val) {
                        std::wstring valStr = L"(" + std::to_wstring(val) + L")";
                        for (size_t i = 0; i < fExpr.length(); ++i) {
                            if (fExpr[i] == varChar) {
                                bool startOk = (i == 0) || (!iswalnum(fExpr[i-1]) && fExpr[i-1] != L'_');
                                bool endOk = (i == fExpr.length() - 1) || (!iswalnum(fExpr[i+1]) && fExpr[i+1] != L'_');
                                if (startOk && endOk) {
                                    fExpr.replace(i, 1, valStr);
                                    i += valStr.length() - 1;
                                }
                            }
                        }
                    };

                    if (args.size() > 0) replaceVar(L'x', args[0]);
                    if (args.size() > 1) replaceVar(L'y', args[1]);
                    if (args.size() > 2) replaceVar(L'z', args[2]);
                    
                    size_t newPos = 0;
                    return parseExpression(fExpr, newPos);
                }
            }
            return 0.0;
        }

        if (id == L"pi") return 3.1415926;
        return 0.0;
    }
    return parseNumber(expr, pos);
}

/**
 * @brief 表达式解析辅助函数 - 解析项（乘除法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的项值
 */
double parseTerm(const std::wstring& expr, size_t& pos)
{
    double value = parseFactor(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'*' || expr[pos] == L'/'))
    {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseFactor(expr, pos);
        
        if (op == L'*')
        {
            value *= nextValue;
        }
        else if (op == L'/' && nextValue != 0)
        {
            value /= nextValue;
        }
        else
        {
            LogToFile("parseTerm: 除零错误");
            throw std::exception("除零错误");
        }
    }
    
    return value;
}

/**
 * @brief 表达式解析辅助函数 - 解析表达式（加减法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的表达式值
 */
double parseExpression(const std::wstring& expr, size_t& pos)
{
    double value = parseTerm(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'+' || expr[pos] == L'-'))
    {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseTerm(expr, pos);
        
        if (op == L'+')
        {
            value += nextValue;
        }
        else
        {
            value -= nextValue;
        }
    }
    
    return value;
}

/**
 * @brief 检查字符串是否是数学表达式
 * @param expression 要检查的字符串
 * @return 如果是数学表达式返回true，否则返回false
 */
bool IsMathExpression(const std::wstring& expression)
{
    // 简单的数学表达式检查：包含数字和运算符
    if (expression.empty()) return false;
    
    // 检查是否包含数学运算符
    const std::wstring operators = L"+-*/^%()";
    for (wchar_t c : expression)
    {
        if (operators.find(c) != std::wstring::npos)
        {
            return true;
        }
    }
    
    // 检查是否包含数字
    for (wchar_t c : expression)
    {
        if (c >= L'0' && c <= L'9')
        {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 计算数学表达式
 * @param expression 要计算的表达式
 * @return 计算结果字符串
 */
std::wstring CalculateExpression(const std::wstring& expression)
{
    // 简单的表达式计算实现
    // 这里应该使用更复杂的数学表达式解析器
    // 目前只实现基本的四则运算
    
    try
    {
        // 将wstring转换为string
        std::string expr(expression.begin(), expression.end());
        
        // 这里应该使用数学表达式计算库
        // 暂时返回一个简单的计算结果
        return L"计算结果: " + expression;
    }
    catch (...)
    {
        return L"计算错误";
    }
}

/**
 * @brief 评估表达式并处理计算逻辑
 * @param expression 要评估的表达式
 */


/**
 * @brief 进入计算模式
 */
void EnterCalculatorMode()
{
    g_currentViewMode = ViewMode::CALCULATOR;
    g_calculatorMode = true;
    
    // 加载自定义公式
    LoadCustomFormulas();
    
    // 加载计算历史记录
    LoadCalculationHistory();
    
    // 注入公式到WebView
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 显示计算历史记录
    DisplayCalculationHistory();
    
    // 添加提示信息
    AddHintRowToListView(L"💡 计算模式：输入数学表达式进行计算，以#开头添加注释");
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新计算模式WebView显示
    UpdateCalculatorModeWebView();
    
    // 注入公式到WebView
    InjectCustomFormulasToWebView();
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("EnterCalculatorMode: 进入计算模式");
}

/**
 * @brief 退出计算模式
 */
void ExitCalculatorMode()
{
    g_calculatorMode = false;
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空ListView
    ClearListView();
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("ExitCalculatorMode: 退出计算模式");
}

/**
 * @brief 显示计算历史记录
 */
void DisplayCalculationHistory()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return;
    }
    
    // 清空ListView
    ClearListView();
    
    // 添加历史记录到ListView
    for (const auto& record : g_calculationHistory)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = ListView_GetItemCount(g_hListView);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(record.expression.c_str());
        ListView_InsertItem(g_hListView, &lvItem);
        
        // 设置第二列（结果）
        lvItem.iSubItem = 1;
        lvItem.pszText = const_cast<LPWSTR>(record.result.c_str());
        ListView_SetItem(g_hListView, &lvItem);
    }
    
    LogToFile("DisplayCalculationHistory: 显示计算历史记录");
}

/**
 * @brief 保存计算历史记录到文件
 */
void SaveCalculationHistory()
{
    WCHAR dataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dataPath) == S_OK)
    {
        wcscat_s(dataPath, L"\\FunnyQuick");
        CreateDirectoryW(dataPath, NULL);
        WCHAR historyPath[MAX_PATH];
        wcscpy_s(historyPath, dataPath);
        wcscat_s(historyPath, L"\\calculation_history.txt");
        
        std::wstring content;
        for (const auto& record : g_calculationHistory)
        {
            content += record.timestamp + L"|" + record.expression + L"|" + record.result + L"|" + record.comment + L"\n";
        }
        
        if (WriteStringToFile(historyPath, content))
        {
            LogToFile("SaveCalculationHistory: 成功保存计算历史记录");
        }
        else
        {
            LogToFile("SaveCalculationHistory: 无法打开历史文件进行写入");
        }
    }
}

/**
 * @brief 从文件加载计算历史记录
 */
void LoadCalculationHistory()
{
    g_calculationHistory.clear();
    WCHAR dataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dataPath) == S_OK)
    {
        wcscat_s(dataPath, L"\\FunnyQuick");
        WCHAR historyPath[MAX_PATH];
        wcscpy_s(historyPath, dataPath);
        wcscat_s(historyPath, L"\\calculation_history.txt");

        std::wstring content = ReadFileToString(historyPath);
        if (content.empty())
        {
            LogToFile("LoadCalculationHistory: 无法打开历史文件或文件为空");
            return;
        }

        std::wstringstream ss(content);
        std::wstring line;
        while (std::getline(ss, line))
        {
            if (line.empty()) continue;

            std::wstringstream lss(line);
            std::wstring segment;
            std::vector<std::wstring> segs;

            while (std::getline(lss, segment, L'|'))
            {
                segs.push_back(segment);
            }

            if (segs.size() >= 3)
            {
                CalculationRecord record;
                record.timestamp = segs[0];
                record.expression = segs[1];
                record.result = segs[2];
                if (segs.size() >= 4)
                {
                    record.comment = segs[3];
                }
                g_calculationHistory.push_back(record);
            }
        }

        LogToFile("LoadCalculationHistory: 成功加载计算历史记录");
    }
    else
    {
        LogToFile("LoadCalculationHistory: 无法获取应用数据目录路径");
    }
}

/**
 * @brief 显示计算器帮助信息
 */
void ShowCalculatorHelpInfo()
{
    // 清空ListView
    ClearListView();
    
    // 添加帮助信息
    const WCHAR* helpItems[] = {
        L"💡 计算模式使用说明",
        L"• 输入数学表达式进行计算（如：2+3*4）",
        L"• 以#开头添加注释（如：#这是注释）",
        L"• 支持基本的四则运算：+ - * /",
        L"• 计算结果会自动保存到历史记录",
        L"• 点击'退出计算'返回普通模式"
    };
    
    for (int i = 0; i < sizeof(helpItems) / sizeof(helpItems[0]); i++)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(helpItems[i]);
        ListView_InsertItem(g_hListView, &lvItem);
    }
    
    LogToFile("ShowCalculatorHelpInfo: 显示计算器帮助信息");
}

/**
 * @brief 评估数学表达式并计算结果
 * @param expression 要计算的表达式字符串
 */
void EvaluateExpression(const WCHAR* expression)
{
    if (!expression || wcslen(expression) == 0)
    {
        return;
    }

    std::wstring expr = expression;
    std::wstring comment;
    size_t hashPos = expr.find(L'#');
    if (hashPos != std::wstring::npos)
    {
        comment = expr.substr(hashPos + 1);
        expr = expr.substr(0, hashPos);
        size_t start = comment.find_first_not_of(L' ');
        size_t end = comment.find_last_not_of(L' ');
        if (start != std::wstring::npos && end != std::wstring::npos)
        {
            comment = comment.substr(start, end - start + 1);
        }
        else
        {
            comment.clear();
        }
    }

    size_t equalPos = expr.find(L'=');
    if (equalPos != std::wstring::npos)
    {
        expr = expr.substr(0, equalPos);
    }

    expr.erase(std::remove(expr.begin(), expr.end(), L' '), expr.end());

    // Check if expression matches a custom formula name exactly (case-insensitive)
    for (const auto& f : g_customFormulas) {
        if (_wcsicmp(f.name.c_str(), expr.c_str()) == 0) {
            EnterFormulaWizardMode(f.name); // Use the correct case name
            return;
        }
    }

    if (g_webView)
    {
        g_pendingComment = comment;
        g_lastEvaluatedExpr = expr;
        InjectCustomFormulasToWebView();
        EvaluateJSExpression(expr);
        return;
    }

    try
    {
        double result = 0.0;
        bool success = false;

        try
        {
            bool isPureNumber = true;
            for (wchar_t c : expr)
            {
                if (!isdigit(c) && c != L'.' && c != L'-')
                {
                    isPureNumber = false;
                    break;
                }
            }

            if (isPureNumber)
            {
                result = std::stod(expr);
                success = true;
            }
            else
            {
                throw std::exception();
            }
        }
        catch (...)
        {
            try
            {
                size_t pos = 0;
                result = parseExpression(expr, pos);
                success = true;
            }
            catch (...)
            {
                success = false;
            }
        }

        if (success)
        {
            WCHAR resultStr[256] = {0};
            swprintf(resultStr, sizeof(resultStr)/sizeof(WCHAR), L"%.6g", result);

            std::wstring displayExpr = expr;
            displayExpr += L" = ";
            displayExpr += resultStr;

            CalculationRecord record;
            record.expression = displayExpr;
            record.result = resultStr;
            record.comment = comment;
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                WCHAR timestamp[64];
                swprintf_s(timestamp, L"%04d-%02d-%02d %02d:%02d:%02d",
                           st.wYear, st.wMonth, st.wDay,
                           st.wHour, st.wMinute, st.wSecond);
                record.timestamp = timestamp;
            }

            g_calculationHistory.push_back(record);
            if (g_calculationHistory.size() > 50)
            {
                g_calculationHistory.erase(g_calculationHistory.begin());
            }

            SaveCalculationHistory();
            DisplayCalculationHistory();
            UpdateCalculatorModeWebView();

            g_updatingEditBox = true;
            SetWindowTextW(g_hEdit, resultStr);
            g_updatingEditBox = false;
            SendMessageW(g_hEdit, EM_SETSEL, 0, -1);
        }
        else
        {
            MessageBoxW(g_hMainWindow, L"无法计算表达式", L"计算错误", MB_OK | MB_ICONERROR);
        }
    }
    catch (...)
    {
        MessageBoxW(g_hMainWindow, L"表达式计算异常", L"计算错误", MB_OK | MB_ICONERROR);
    }
}
void OnCalculationResult(const std::wstring& expression, const std::wstring& result)
{
    LogToFile("OnCalculationResult: 收到计算结果");
    std::string exprStr(expression.begin(), expression.end());
    std::string resStr(result.begin(), result.end());
    char logMsg[512];
    sprintf(logMsg, "Expression: %s, Result: %s", exprStr.c_str(), resStr.c_str());
    LogToFile(logMsg);

    if (result.empty() || (result.size() >= 5 && wcsncmp(result.c_str(), L"Error", 5) == 0))
    {
        MessageBoxW(g_hMainWindow, L"无法计算表达式", L"计算错误", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring expr = expression;

    std::wstring displayExpr = expr;
    displayExpr += L" = ";
    displayExpr += result;

    CalculationRecord record;
    record.expression = displayExpr;
    record.result = result;
    record.comment = g_pendingComment;
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        WCHAR timestamp[64];
        swprintf_s(timestamp, L"%04d-%02d-%02d %02d:%02d:%02d",
                   st.wYear, st.wMonth, st.wDay,
                   st.wHour, st.wMinute, st.wSecond);
        record.timestamp = timestamp;
    }

    g_calculationHistory.push_back(record);
    if (g_calculationHistory.size() > 50)
    {
        g_calculationHistory.erase(g_calculationHistory.begin());
    }

    SaveCalculationHistory();

    if (g_currentViewMode == ViewMode::FORMULA_WIZARD) {
        // Update wizard UI
        std::wstring escapedResult;
        for (wchar_t c : result) {
            if (c == L'\\') escapedResult += L"\\\\";
            else if (c == L'\'') escapedResult += L"\\'";
            else if (c == L'\n') escapedResult += L"\\n";
            else escapedResult += c;
        }
        std::wstring script = L"if(window.showResult) window.showResult('" + escapedResult + L"');";
        g_webView->ExecuteScript(script.c_str(), nullptr);
    } else {
        DisplayCalculationHistory();
        UpdateCalculatorModeWebView();

        g_updatingEditBox = true;
        SetWindowTextW(g_hEdit, result.c_str());
        g_updatingEditBox = false;
        SendMessageW(g_hEdit, EM_SETSEL, 0, -1);
    }

    g_pendingComment.clear();
}
