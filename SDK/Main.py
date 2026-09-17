import os
    
class SDKFixer:
    def __init__(self):
        self.sdk_folder = os.path.join(os.getcwd(), "SDK")
        self.files_to_fix = [
            "GameplayTags_structs.hpp",
            "PhysicsCore_classes.hpp",
        ]
    
    def fix_sdk(self):
        for filename in self.files_to_fix:
            filepath = os.path.join(self.sdk_folder, filename)
            
            if not os.path.exists(filepath):
                print(f"Couldn't find {filepath} - skipping")
                continue
            
            with open(filepath, 'r') as f:
                code = f.read()
    
            if "GameplayTags" in filename:
                code = self.Fix_GameplayTags(code)
            elif "PhysicsCore" in filename:
                code = self.Fix_PhysicsCore(code)
    
            with open(filepath, 'w') as f:
                f.write(code)
            
            print(f"Fixed {filename}")
    
    def Fix_PhysicsCore(self, code):
        fixes = [
            ("class UChaosServerCollisionDebugSubsystem final : public UTickableWorldSubsystem", 
                "class UChaosServerCollisionDebugSubsystem final : public UObject"),
            ("static_assert(sizeof(UChaosServerCollisionDebugSubsystem)", 
                "//static_assert(sizeof(UChaosServerCollisionDebugSubsystem)")
        ]
        for Broken, Fixed in fixes:
            code = code.replace(Broken, Fixed)
        return code
    
    def Fix_GameplayTags(self, code):
        fixes = [
            ("struct FGameplayTagTableRow : public FTableRowBase", 
                "struct FGameplayTagTableRow //: public FTableRowBase"),
            ("static_assert(sizeof(FGameplayTagTableRow)", 
                "//static_assert(sizeof(FGameplayTagTableRow)"),
            ("static_assert(offsetof(FGameplayTagTableRow, Tag)", 
                "//static_assert(offsetof(FGameplayTagTableRow, Tag)"),
            ("static_assert(offsetof(FGameplayTagTableRow, DevComment)", 
                "//static_assert(offsetof(FGameplayTagTableRow, DevComment)"),
            ("static_assert(sizeof(FRestrictedGameplayTagTableRow)", 
                "//static_assert(sizeof(FRestrictedGameplayTagTableRow)"),
            ("static_assert(offsetof(FRestrictedGameplayTagTableRow, bAllowNonRestrictedChildren)", 
                "//static_assert(offsetof(FRestrictedGameplayTagTableRow, bAllowNonRestrictedChildren)"),
            ("static_assert(alignof(FGameplayTagContainer)", 
                "//static_assert(alignof(FGameplayTagContainer)")
        ]
        for Broken, Fixed in fixes:
            code = code.replace(Broken, Fixed)
        return code
    
fixer = SDKFixer()
fixer.fix_sdk()