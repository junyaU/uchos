#!/bin/bash

# UCHos命名規則チェックスクリプト（全体版）
# Google C++ Style Guide準拠の命名規則を全プロジェクトに対してチェック

echo "=== UCHos命名規則チェック（全体版） ==="
echo "対象範囲: kernel/, libs/, userland/ 全体"

# カウンタ初期化
violations=0
warnings=0

# 色付きの出力
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 結果表示関数
log_violation() {
    echo -e "${RED}[VIOLATION]${NC} $1"
    ((violations++))
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
    ((warnings++))
}

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_section() {
    echo -e "${BLUE}[SECTION]${NC} $1"
}

# チェック対象ディレクトリ
TARGET_DIRS=("kernel" "libs" "userland")

# snake_case型名のチェック（Google C++ Style Guide準拠ではPascalCaseにすべき）
check_snake_case_types() {
    local dir=$1
    local section_name=$2
    
    log_section "1. snake_case型名チェック - $section_name"
    
    local snake_case_files=$(find "$dir" -name "*.hpp" -o -name "*.cpp" 2>/dev/null | \
        xargs grep -l -E '^\s*(struct|class|enum\s+class)\s+[a-z][a-z_]*[a-z]\s*\{' 2>/dev/null || true)
    
    if [ -z "$snake_case_files" ]; then
        log_info "✓ $section_name でsnake_case型は見つかりませんでした"
    else
        # 一時ファイルでviolationを収集
        local temp_violations=$(mktemp)
        
        # 各ファイルをチェックしてviolationを収集
        for file in $snake_case_files; do
            if [ -n "$file" ]; then
                # ファイル内のsnake_case型をチェック（rebindを除外）
                local violations=$(grep -n -E '^\s*(struct|class|enum\s+class)\s+[a-z][a-z_]*[a-z]\s*\{' "$file" 2>/dev/null | \
                    grep -v "struct rebind" | head -3)
                if [ -n "$violations" ]; then
                    echo "$violations" | sed "s|^|$file:|" >> "$temp_violations"
                fi
            fi
        done
        
        # violationの結果を確認
        if [ -s "$temp_violations" ]; then
            local count=$(echo "$snake_case_files" | wc -l)
            log_violation "$section_name で $count 個のファイルにsnake_case型があります"
            while IFS= read -r line; do
                log_violation "  $line"
            done < "$temp_violations"
        else
            log_info "✓ $section_name でsnake_case型は見つかりませんでした（C++標準要件rebindは除外）"
        fi
        
        # 一時ファイルを削除
        rm -f "$temp_violations"
    fi
    echo
}

# PascalCase関数・変数名のチェック（Google C++ Style Guide準拠では snake_case にすべき）
check_pascal_case_functions_vars() {
    local dir=$1
    local section_name=$2
    
    log_section "2. PascalCase関数・変数名チェック - $section_name"
    
    local pascal_case_matches=$(find "$dir" -name "*.hpp" -o -name "*.cpp" 2>/dev/null | \
        xargs grep -n -E '^\s*[a-z_][a-z0-9_]*\s+[A-Z][a-zA-Z0-9]*\s*\(' 2>/dev/null | \
        grep -v -E '(class\s+|struct\s+|::|operator|template|namespace)' || true)
    
    if [ -z "$pascal_case_matches" ]; then
        log_info "✓ $section_name でPascalCase関数名は見つかりませんでした"
    else
        local count=$(echo "$pascal_case_matches" | wc -l)
        log_warning "$section_name で $count 個のPascalCase関数名の可能性があります"
        
        echo "$pascal_case_matches" | head -5 | while IFS= read -r line; do
            if [ -n "$line" ]; then
                log_warning "  $line"
            fi
        done
    fi
    echo
}

# ALL_CAPS以外のマクロをチェック
check_macro_naming() {
    local dir=$1
    local section_name=$2
    
    log_section "3. マクロ命名チェック - $section_name"
    
    local non_caps_macros=$(find "$dir" -name "*.hpp" -o -name "*.cpp" 2>/dev/null | \
        xargs grep -n -E '^\s*#define\s+[a-zA-Z_][a-zA-Z0-9_]*' 2>/dev/null | \
        grep -v -E '#define\s+[A-Z][A-Z0-9_]*(\s|$)' || true)
    
    if [ -z "$non_caps_macros" ]; then
        log_info "✓ $section_name で非ALL_CAPSマクロは見つかりませんでした"
    else
        local count=$(echo "$non_caps_macros" | wc -l)
        log_warning "$section_name で $count 個の非ALL_CAPSマクロがあります"
        
        echo "$non_caps_macros" | head -5 | while IFS= read -r line; do
            if [ -n "$line" ]; then
                log_warning "  $line"
            fi
        done
    fi
    echo
}

# Doxygenコメントでのsnake_case型名チェック
check_doxygen_snake_case() {
    local dir=$1
    local section_name=$2
    
    log_section "4. Doxygenコメントsnake_case型名チェック - $section_name"
    
    local doxygen_snake_case=$(find "$dir" -name "*.hpp" -o -name "*.cpp" 2>/dev/null | \
        xargs grep -n -E '@(struct|class|enum)\s+[a-z][a-z_]*[a-z]' 2>/dev/null || true)
    
    if [ -z "$doxygen_snake_case" ]; then
        log_info "✓ $section_name のDoxygenコメントでsnake_case型名は見つかりませんでした"
    else
        local count=$(echo "$doxygen_snake_case" | wc -l)
        log_warning "$section_name のDoxygenコメントで $count 個のsnake_case型名があります"
        
        echo "$doxygen_snake_case" | head -5 | while IFS= read -r line; do
            if [ -n "$line" ]; then
                log_warning "  $line"
            fi
        done
    fi
    echo
}

# ハードウェア関連の重要型チェック（kernel限定）
check_important_hardware_types() {
    if [ ! -d "kernel/hardware" ]; then
        return
    fi
    
    log_section "5. 重要なハードウェア型の確認"
    
    local expected_types=(
        "Device" "ClassDriver" "Controller" "DeviceManager"
        "VirtqDesc" "VirtioBlkReq" "VirtioPciDevice" 
        "EndpointDescriptor" "EndpointConfig" "EndpointId"
        "CapabilityRegisters" "OperationalRegisters"
        "MemoryMappedRegister" "ArrayWrapper" "SlotState"
    )
    
    local found_types=0
    
    for type_name in "${expected_types[@]}"; do
        if find kernel/hardware/ -name "*.hpp" -exec grep -q -E "(struct|class)\s+$type_name" {} \; 2>/dev/null; then
            ((found_types++))
        fi
    done
    
    local expected_count=${#expected_types[@]}
    local percentage=$(( found_types * 100 / expected_count ))
    
    if [ $percentage -ge 80 ]; then
        log_info "✓ ハードウェア関連重要型: $found_types/$expected_count 個 (${percentage}%) 確認済み"
    else
        log_warning "ハードウェア関連重要型: $found_types/$expected_count 個 (${percentage}%) のみ確認"
    fi
    echo
}

# 統計情報の表示
show_statistics() {
    log_section "統計情報"
    
    for dir in "${TARGET_DIRS[@]}"; do
        if [ -d "$dir" ]; then
            local file_count=$(find "$dir" -name "*.hpp" -o -name "*.cpp" 2>/dev/null | wc -l)
            local struct_count=$(find "$dir" -name "*.hpp" -exec grep -c -E '^\s*struct\s+[A-Z]' {} + 2>/dev/null | \
                awk '{s+=$1} END {print s+0}')
            local class_count=$(find "$dir" -name "*.hpp" -exec grep -c -E '^\s*class\s+[A-Z]' {} + 2>/dev/null | \
                awk '{s+=$1} END {print s+0}')
            
            log_info "$dir/: ファイル数=$file_count, PascalCase構造体=$struct_count, PascalCaseクラス=$class_count"
        fi
    done
    echo
}

# メイン実行
echo "チェック開始時刻: $(date)"
echo

# 各ディレクトリをチェック
for dir in "${TARGET_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        log_warning "ディレクトリが見つかりません: $dir"
        continue
    fi
    
    echo "==================== $dir/ ===================="
    check_snake_case_types "$dir" "$dir"
    check_pascal_case_functions_vars "$dir" "$dir"
    check_macro_naming "$dir" "$dir"
    check_doxygen_snake_case "$dir" "$dir"
done

# ハードウェア関連の特別チェック
check_important_hardware_types

# 統計情報
show_statistics

# 結果表示
echo "==================== チェック結果 ===================="
echo "チェック完了時刻: $(date)"

if [ $violations -eq 0 ] && [ $warnings -eq 0 ]; then
    echo -e "${GREEN}✅ 全体命名規則チェック完了: 問題は見つかりませんでした${NC}"
    echo "  - kernel/: Google C++ Style Guide準拠"
    echo "  - libs/: Google C++ Style Guide準拠"
    echo "  - userland/: Google C++ Style Guide準拠"
    exit 0
elif [ $violations -eq 0 ]; then
    echo -e "${YELLOW}⚠️  全体命名規則チェック完了: ${warnings}件の軽微な警告がありました${NC}"
    echo "  詳細は上記の出力を確認してください"
    exit 0
else
    echo -e "${RED}❌ 命名規則違反: ${violations}件の違反と${warnings}件の警告が見つかりました${NC}"
    echo "  修正が必要な項目があります"
    exit 1
fi