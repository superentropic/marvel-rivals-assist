#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace SkinChanger {

    enum SkinRarity { SR_COMMON = 0, SR_RARE, SR_EPIC, SR_LEGENDARY };

    static std::unordered_map<int32_t, std::string> HeroNames = {
        {1011, "Hulk"}, {1014, "The Punisher"}, {1015, "Storm"}, {1016, "Loki"},
        {1017, "Human Torch"}, {1018, "Doctor Strange"}, {1020, "Mantis"}, {1021, "Hawkeye"},
        {1022, "Captain America"}, {1023, "Rocket Raccoon"}, {1024, "Hela"}, {1025, "Cloak & Dagger"},
        {1026, "Black Panther"}, {1027, "Groot"}, {1028, "Ultron"}, {1029, "Magik"},
        {1030, "Moon Knight"}, {1031, "Luna Snow"}, {1032, "Squirrel Girl"}, {1033, "Black Widow"},
        {1034, "Iron Man"}, {1035, "Venom"}, {1036, "Spider-Man"}, {1037, "Magneto"},
        {1038, "Scarlet Witch"}, {1039, "Thor"}, {1040, "Mister Fantastic"}, {1041, "Winter Soldier"},
        {1042, "Peni Parker"}, {1043, "Star-Lord"}, {1044, "Blade"}, {1045, "Namor"},
        {1046, "Adam Warlock"}, {1047, "Jeff the Land Shark"}, {1048, "Psylocke"},
        {1049, "Wolverine"}, {1050, "Invisible Woman"}, {1051, "The Thing"},
        {1052, "Iron Fist"}, {1053, "Emma Frost"}, {1054, "Phoenix"},
        {1055, "Daredevil"}, {1056, "Angela"}, {1057, "Deadpool"}, {1058, "Gambit"},
        {1059, "Elsa Bloodstone"}, {1065, "Rogue"}
    };

    static std::unordered_map<int32_t, std::unordered_map<int32_t, std::string>> SkinNames = {
        {1011, {{1, "Default"}, {100, "Mighty G-Bomb"}, {300, "Maestro"}, {500, "Punk Rage"},
                {501, "Green Scar"}, {502, "Joe Fixit"}}},
        {1014, {{1, "Default"}, {100, "Camo"}, {300, "Dangan Ronin"}, {500, "Thunderbolts"},
                {501, "Punisher 2099"}, {502, "Aqua Arsenal"}, {503, "Franken-Castle"}, {504, "Emerald Executioner"},
                {505, "Amber Annihilator"}, {800, "Daredevil: Born Again"}}},
        {1015, {{1, "Default"}, {100, "Ivory Breeze"}, {300, "Judicator Xiezhi"}, {500, "Mohawk Rock"},
                {501, "Ultimate Wind-Rider"}, {502, "Goddess of Thunder"}, {503, "Symbiote Storm"}, {504, "Queen of Wakanda"}}},
        {1016, {{1, "Default"}, {100, "Frost Giant"}, {101, "IGNITE Loki (2025)"}, {300, "Shin Sagi-Shi"},
                {301, "Robe of Relaxation"}, {302, "Immortal Firebird"}, {303, "Tidal Trickery"}, {500, "Presidential Attire"},
                {501, "All-Butcher"}, {502, "Lady Loki"}, {800, "Loki Season 2"}}},
        {1017, {{1, "Default"}, {100, "First Family"}, {101, "Blood Blaze"}, {300, "Jack of Hearts"},
                {500, "Negative Zone Gladiator"}, {501, "Future Foundation"}, {502, "Sunny Sizzler"}, {504, "Indigo Inferno"},
                {800, "The Fantastic Four: First Steps"}}},
        {1018, {{1, "Default"}, {100, "Master of Black Magic"}, {101, "Astral Wanderer"}, {300, "Sorcerer Immortal"},
                {301, "Old Man Strange"}, {302, "Bleeker Street Strut"}, {303, "Phantom Sorcerer"}, {500, "God of Magic"},
                {501, "Sorcerer Supreme of the Galaxy"}, {800, "Doctor Strange in the Multiverse of Madness"}}},
        {1020, {{1, "Default"}, {100, "Knowhere Corp"}, {101, "Will of Galacta"}, {300, "Galactic Wings"},
                {301, "Jade Maiden"}, {302, "Oceanic Harmony"}, {303, "Blue Breeze"}, {304, "Citrus Sunrise"},
                {305, "Galactic Gladiator"}, {500, "Flora Maiden"}, {800, "Guardians of the Galaxy Vol. 3"}}},
        {1021, {{1, "Default"}, {100, "Tiger's Eye"}, {101, "Will of Galacta"}, {300, "Galactic Fangs"},
                {301, "Old Man Hawkeye"}, {500, "Ronin"}, {501, "Freefall"}, {502, "Binary Arrow"}}},
        {1022, {{1, "Default"}, {100, "Captain A.I.M.erica"}, {300, "Galactic Talon"}, {500, "Captain Gladiator"},
                {501, "Star Spangled Style"}, {502, "Captain Klyntar"}, {503, "Golden Age"}, {504, "Capwolf"},
                {801, "Avengers: Infinity War"}}},
        {1023, {{1, "Default"}, {100, "Rocky"}, {101, "Will of Galacta"}, {300, "Bounty Hunter"},
                {302, "Sunshine Raccoon"}, {303, "Radiant Reef"}, {304, "Bluebell Breeze"}, {305, "Rocket of the Rafters"},
                {306, "Giant Panda"}, {500, "Symbiote Raccoon"}, {801, "Guardians of the Galaxy Vol. 3"}}},
        {1024, {{1, "Default"}, {100, "Ultimate"}, {101, "Will of Galacta"}, {301, "Empress of the Cosmos"},
                {302, "Yami no Karasu"}, {303, "Queen in Black"}, {304, "The Grim Lady"}, {305, "Disco of the Dead"},
                {500, "Goddess of Death"}, {800, "Merciful Queen"}}},
        {1025, {{1, "Default"}, {100, "Lemon Lime"}, {300, "Dance Partner"}, {301, "Twilight Duo"},
                {302, "Polarity Bond"}, {303, "Ice Pas de Deux"}, {500, "Growth & Decay"}}},
        {1026, {{1, "Default"}, {100, "Orisha Blood"}, {101, "Golden Panther"}, {300, "Galactic Claw"},
                {301, "Thrice-Cursed King"}, {302, "Phoenix Panther"}, {500, "Bast's Chosen"}, {501, "Damisa-Sarki"},
                {502, "King of Wakanda"}}},
        {1027, {{1, "Default"}, {100, "Abies Algae"}, {301, "Yggroot"}, {302, "Holiday Happiness"},
                {303, "Mecha-Flora"}, {500, "Carved Traveler"}, {501, "Symbiote Flora"}, {502, "Big Buddy"},
                {800, "Guardians of the Galaxy Vol. 3"}}},
        {1028, {{1, "Default"}, {100, "Mechanical Phantom"}, {101, "Golden Ultron"}, {102, "Will of Galacta"},
                {300, "Wasteland Robot"}, {500, "X-Tron"}, {800, "Infinity Ultron"}}},
        {1029, {{1, "Default"}, {100, "Amethyst Armor"}, {101, "Will of Galacta"}, {300, "Punkchild"},
                {301, "Frozen Demon"}, {302, "Rosy Resilience"}, {303, "New Millennia Might"}, {500, "Eldritch Armor"},
                {501, "Retro X-Uniform"}, {502, "Phoenix Demon"}}},
        {1030, {{1, "Default"}, {100, "Golden Moonlight"}, {101, "Blood Moon Knight"}, {300, "Lunar General"},
                {301, "King of Clubs"}, {302, "Eclipse Knight"}, {500, "Mister Knight"}, {501, "Phoenix Knight"},
                {800, "Fist Of Vengeance"}, {801, "Moon Knight Mech"}}},
        {1031, {{1, "Default"}, {100, "Minty Beats"}, {300, "Shining Star"}, {301, "Mirae 2099"},
                {302, "Nolaehaneun Manyeo"}, {303, "Cool Summer"}, {304, "Plasma Pulse"}, {305, "Abyssal Glow"},
                {306, "Radiant Radiance"}, {307, "Prismatic Pulse"}, {308, "Cherry Delight"}, {309, "Disco Pop"},
                {310, "Blueberry Ice"}, {311, "Fruit Cake Flurry"}, {314, "Night Nebula"}, {315, "Jade Jewel"}}},
        {1032, {{1, "Default"}, {100, "Arctic Lemmus"}, {300, "Nut Rocker"}, {301, "Cheerful Dragoness"},
                {302, "Sunshine Squirrel"}, {303, "Turbo Tailwind"}, {304, "Tinsel Tail"}, {305, "Red Panda"},
                {500, "Urban Hunter"}, {501, "Symbiote Squirrel"}}},
        {1033, {{1, "Default"}, {100, "Lethal Toxicity"}, {300, "Lion's Heartbeat"}, {500, "Red Runway Veil"},
                {501, "Mrs. Barnes"}, {502, "Phoenix Widow"}, {503, "Midnight Suspense"}, {800, "White Suit"}}},
        {1034, {{1, "Default"}, {100, "Armor Model 42"}, {300, "Blood Edge Armor"}, {301, "Iron Mariner"},
                {500, "Steam Power"}, {501, "Superior Iron Man"}, {502, "Extrembiote Armor"}, {503, "Big Shot"},
                {504, "Mark I"}, {800, "Avengers: Endgame"}}},
        {1035, {{1, "Default"}, {100, "Cyan Clash"}, {101, "Anti-Venom"}, {102, "Hyper Orange"},
                {103, "Pink Bubble"}, {300, "Snow Symbiote"}, {301, "Reborn King in Black"}, {302, "Gummy Surprise"},
                {303, "Space Corsair"}, {304, "Phage Palette"}, {305, "Frosted Agony"}, {500, "Lingering Imprint"},
                {501, "Space Knight"}, {800, "Marvel Cosmic Invasion"}}},
        {1036, {{1, "Default"}, {100, "Scarlet Spider"}, {101, "Chasm"}, {102, "Black & Gold"},
                {300, "Spider-Oni"}, {500, "Spider-Punk 2099"}, {501, "Bag-Man Beyond"}, {502, "Black Suit"},
                {503, "Marvel's Spider-Man 2"}, {504, "Future Foundation"}, {505, "Man-Spider"}, {508, "Iron Spider"},
                {509, "Groovy Swing"}, {800, "Spider-Man: No Way Home"}, {801, "Marvel Cosmic Invasion"}}},
        {1037, {{1, "Default"}, {100, "Uncanny Blacksteel"}, {101, "Will of Galacta"}, {102, "Black & Gold"},
                {300, "Binary Sword"}, {301, "Temporal Tyrant"}, {302, "The Trial of Magneto"}, {500, "Master of Magnetism"},
                {501, "King Magnus"}, {502, "Seat of Autumn"}}},
        {1038, {{1, "Default"}, {100, "White Witch"}, {101, "Nyx Weaver"}, {300, "Immortal Sovereign"},
                {301, "Phoenix Chaos"}, {302, "Majestic Mauve"}, {303, "Neon Nimbus"}, {304, "Twisted Conjurer"},
                {305, "Frostbitten Witch"}, {306, "Deep Green Magic"}, {500, "Chaos Gown"}, {501, "Emporium Matron"},
                {502, "Witch of the Evil Eye"}, {800, "Doctor Strange in the Multiverse of Madness"}, {801, "The Queen Of The Dead"}}},
        {1039, {{1, "Default"}, {100, "Midgard Umber"}, {300, "Worthy Waves"}, {301, "Lightning Fast"},
                {302, "Azure Skies"}, {303, "God of Winter"}, {500, "Herald of Thunder"}, {501, "Reborn from Ragnarok"},
                {502, "Lord Of Asgard"}, {503, "Majestic Raiment"}, {504, "Boogie Bolt"}, {507, "Shadow Shock"},
                {508, "Purple Pulse"}, {800, "Thor: Love and Thunder"}}},
        {1040, {{1, "Default"}, {100, "First Family"}, {101, "Will of Galacta"}, {300, "The Life Fantastic"},
                {301, "Dad-tastic Reed"}, {500, "The Maker"}, {501, "Future Foundation"}, {800, "The Fantastic Four: First Steps"}}},
        {1041, {{1, "Default"}, {100, "Navy Trooper"}, {300, "Blood Soldier"}, {301, "Polarity Soldier"},
                {302, "Winter's Wrath"}, {500, "Revolution"}, {501, "Winter's Veil"}, {502, "Winter Buckaroo"},
                {503, "Bucky"}, {800, "Thunderbolts*"}}},
        {1042, {{1, "Default"}, {100, "Olive Skimmer"}, {101, "Blue Tarantula"}, {300, "Yatsukahagi"},
                {301, "Wasteland Mech"}, {302, "Floral Frights"}, {303, "Snow-SP//DR"}, {304, "Skeleton"},
                {500, "VEN#M"}}},
        {1043, {{1, "Default"}, {100, "Jovial Star"}, {102, "Ignite Star-Lord (2025)"}, {103, "Luminous Legend"},
                {300, "Lion's Mane"}, {301, "Starcracker"}, {500, "Master of the Sun"}, {501, "King of Spartax"},
                {502, "Starlit Outlaw"}, {800, "Guardians of the Galaxy Vol. 3"}}},
        {1044, {{1, "Default"}, {100, "Daybreak"}, {101, "Will of Galacta"}, {102, "Emerald Blade"},
                {300, "Polarity Edge"}, {301, "Vampire Slayer"}, {500, "Restful Recovery"}, {800, "Blade Knight"}}},
        {1045, {{1, "Default"}, {100, "Mauve Sub-Mariner"}, {101, "Will of Galacta"}, {300, "Phantom Tide"},
                {301, "Monstro King"}, {500, "Savage Sub-Mariner"}, {501, "Retro X-Uniform"}, {502, "Phoenix King"},
                {800, "Black Panther: Wakanda Forever"}}},
        {1046, {{1, "Default"}, {100, "Cosmic Jade"}, {101, "Will of Galacta"}, {102, "King in White"},
                {300, "Immortal Avatar"}, {301, "Blood Soul"}, {302, "Cosmic Warlock"}, {500, "Magus"},
                {800, "Guardians of the Galaxy Vol. 3"}}},
        {1047, {{1, "Default"}, {100, "Adopted Avenger"}, {300, "Cuddly Fuzzlefin"}, {301, "Sunshine Land Shark"},
                {302, "Jeff O'lantern"}, {303, "Verdant Vortex"}, {304, "Blue Barrage"}, {305, "Powder Pink"},
                {306, "Blue Blizzard"}, {307, "Duck Defender"}, {310, "White Waddle"}, {311, "Green Bill"},
                {500, "Incognito Dolphin"}, {501, "Devouring Duo"}, {502, "Business Shark"}, {800, "8-Bit Bash"}}},
        {1048, {{1, "Default"}, {100, "Kirisaki Sakura"}, {300, "Blood Kariudo"}, {302, "Daring Daifuku"},
                {304, "Psychedelic Pulse"}, {305, "Orange Edge"}, {306, "Blue Bolt"}, {307, "Matcha Mirage"},
                {308, "Grape Wagashi"}, {311, "Twilight Tones"}, {312, "Amethyst Aura"}, {500, "Vengeance"},
                {501, "Retro X-Uniform"}, {503, "Phantom Purple"}, {504, "Violet Veil"}, {505, "Moonlit Mirage"}}},
        {1049, {{1, "Default"}, {100, "Lone Wolf"}, {300, "Blood Berserker"}, {301, "Dog Brother X"},
                {500, "Patch"}, {501, "Weapon X"}, {502, "Weapon PhoeniX"}, {503, "Winter Soldier"},
                {800, "Deadpool & Wolverine"}}},
        {1050, {{1, "Default"}, {100, "First Family"}, {101, "Blood Shield"}, {103, "Will of Galacta"},
                {300, "The Life Fantastic"}, {301, "Disappearing Dessert"}, {302, "Prism Parade"}, {303, "Mango Magic"},
                {304, "Alluring Apple"}, {308, "Radiant Ray"}, {309, "Vivid Vibe"}, {500, "Malice"},
                {501, "Future Foundation"}, {502, "Azure Shade"}, {504, "Midnight Majesty"}, {505, "Dune Daisy"},
                {506, "Lush Luminance"}, {507, "Tangerine Tint"}, {800, "The Fantastic Four: First Steps"}}},
        {1051, {{1, "Default"}, {100, "First Family"}, {101, "The Unlimited"}, {102, "Blue Thing"},
                {300, "Rocky Tide"}, {301, "Sunshine Thing"}, {302, "Verdant Vanguard"}, {500, "Trench Coat"},
                {501, "Future Foundation"}, {502, "Symbiote-Thing"}, {800, "The Fantastic Four: First Steps"}}},
        {1052, {{1, "Default"}, {100, "Martial Arts Savant"}, {101, "Will of Galacta"}, {300, "Lion's Gaze"},
                {301, "Binary Fist"}, {302, "Shenloong Champion"}, {304, "Savage Spirit"}, {305, "Lively Lion"},
                {500, "Sword Master"}, {501, "Immortal Weapon of Agamotto"}, {502, "Phoenix Fist"}}},
        {1053, {{1, "Default"}, {100, "Blue Sapphire"}, {101, "Will of Galacta"}, {102, "Golden Diamond"},
                {300, "Hellfire Protocol"}, {301, "Queen of Diamonds"}, {500, "X-Revolution"}, {501, "Phoenix Diamond"},
                {502, "Black Queen of the Marauders"}}},
        {1054, {{1, "Default"}, {100, "The Return of Jean Grey"}, {101, "Emerald Flames"}, {102, "Will of Galacta"},
                {300, "Chaos Phoenix"}, {301, "Ice Phoenix"}, {500, "Dark Phoenix"}, {501, "Seat of Spring"}}},
        {1055, {{1, "Default"}, {100, "Fearless Origin"}, {101, "Shenloong's Creed"}, {102, "Aurora Twilight"},
                {500, "Devil 2099"}, {501, "Not Daredevil"}}},
        {1056, {{1, "Default"}, {100, "Siriana's Silver"}, {101, "Divine Dragon Wing"}, {300, "Ace of Spades"},
                {500, "Skuld 2099"}, {501, "Odin's Beautiful Daughter"}}},
        {1057, {{1, "Default"}, {100, "X-Force?"}, {101, "Workwear Woes"}, {300, "Captain Pool"}}},
        {1058, {{1, "Default"}, {100, "Crimson Heart"}, {101, "Sacrificial Pawn"}, {300, "Thieves Guildmaster"},
                {302, "Mr. X"}}},
        {1059, {{1, "Default"}, {100, "Apex Huntress"}, {101, "Icy Edge"}, {300, "Young Blood"}}},
        {1065, {{1, "Default"}, {100, "Queen's Defense"}, {101, "Searing Spellstripe"}, {300, "Rogue Redux"},
                {301, "Mrs. X"}}}
    };

    inline SkinRarity GetRarity(const std::string& name) {
        static std::unordered_map<std::string, SkinRarity> m = [] {
            std::unordered_map<std::string, SkinRarity> r;
            const char* rare[] = {
                "Abies Algae", "Adopted Avenger", "Amethyst Armor", "Anti-Venom", "Apex Huntress", "Arctic Lemmus",
                "Armor Model 42", "Astral Wanderer", "Aurora Twilight", "Big Buddy", "Blood Blaze", "Blood Moon Knight",
                "Blood Shield", "Blue Sapphire", "Blue Tarantula", "Blue Thing", "Bucky", "Camo",
                "Captain A.I.M.erica", "Carved Traveler", "Chasm", "Cosmic Jade", "Crimson Heart", "Cyan Clash",
                "Daybreak", "Divine Dragon Wing", "Emerald Blade", "Emerald Flames", "Fearless Origin", "First Family",
                "Frost Giant", "Golden Age", "Golden Diamond", "Golden Moonlight", "Golden Panther", "Golden Ultron",
                "Hyper Orange", "Icy Edge", "IGNITE Loki (2025)", "Ignite Star-Lord (2025)", "Iron Spider", "Ivory Breeze",
                "Jovial Star", "King in White", "Kirisaki Sakura", "Knowhere Corp", "Lemon Lime", "Lethal Toxicity",
                "Lone Wolf", "Luminous Legend", "Martial Arts Savant", "Master of Black Magic", "Mauve Sub-Mariner", "Mechanical Phantom",
                "Merciful Queen", "Midgard Umber", "Mighty G-Bomb", "Minty Beats", "Navy Trooper", "Nolaehaneun Manyeo",
                "Nyx Weaver", "Olive Skimmer", "Orisha Blood", "Queen's Defense", "Reborn from Ragnarok", "Retro X-Uniform",
                "Rocky", "Sacrificial Pawn", "Savage Sub-Mariner", "Scarlet Spider", "Searing Spellstripe", "Shenloong Champion",
                "Shenloong's Creed", "Siriana's Silver", "Symbiote Flora", "The Return of Jean Grey", "The Unlimited", "Thunderbolts",
                "Tiger's Eye", "Ultimate", "Uncanny Blacksteel", "Vampire Slayer", "White Witch", "Will of Galacta",
                "Winter's Veil", "Workwear Woes", "X-Force?"
            };
            for (auto s : rare) r[s] = SR_RARE;
            const char* epic[] = {
                "8-Bit Bash", "Ace of Spades", "Azure Skies", "Bag-Man Beyond", "Bast's Chosen", "Binary Arrow",
                "Binary Fist", "Black & Gold", "Black Panther: Wakanda Forever", "Black Queen of the Marauders", "Black Suit", "Blade Knight",
                "Blood Edge Armor", "Blood Kariudo", "Blood Soldier", "Blood Soul", "Blue Blizzard", "Bounty Hunter",
                "Captain Gladiator", "Captain Klyntar", "Captain Pool", "Chaos Gown", "Cheerful Dragoness", "Cuddly Fuzzlefin",
                "Dad-tastic Reed", "Daredevil: Born Again", "Dark Phoenix", "Deadpool & Wolverine", "Devil 2099", "Disco of the Dead",
                "Dog Brother X", "Dune Daisy", "Eclipse Knight", "Eldritch Armor", "Emporium Matron",
                "Empress of the Cosmos", "Fist Of Vengeance", "Flora Maiden", "Franken-Castle", "Freefall", "Frozen Demon",
                "Future Foundation", "Galactic Claw", "Galactic Fangs", "Galactic Gladiator", "Galactic Talon", "Giant Panda",
                "God of Magic", "God of Winter", "Goddess of Death", "Groovy Swing", "Growth & Decay", "Guardians of the Galaxy Vol. 3",
                "Herald of Thunder", "Holiday Happiness", "Immortal Avatar", "Immortal Firebird", "Immortal Weapon of Agamotto", "Incognito Dolphin",
                "Indigo Inferno", "Infinity Ultron", "Jack of Hearts", "Jade Maiden", "Joe Fixit", "Judicator Xiezhi",
                "King Magnus", "King of Clubs", "King of Spartax", "King of Wakanda", "Lightning Fast", "Lingering Imprint",
                "Lion's Heartbeat", "Lion's Mane", "Loki Season 2", "Lord Of Asgard", "Maestro", "Magus",
                "Malice", "Man-Spider", "Mark I", "Marvel Cosmic Invasion", "Marvel's Spider-Man 2", "Master of Magnetism",
                "Master of the Sun", "Midnight Majesty", "Midnight Suspense", "Mister Knight", "Mohawk Rock", "Mrs. Barnes",
                "Negative Zone Gladiator", "Not Daredevil", "Nut Rocker", "Odin's Beautiful Daughter", "Old Man Hawkeye", "Old Man Strange",
                "Patch", "Phantom Purple", "Phantom Sorcerer", "Phoenix Demon", "Phoenix Diamond", "Phoenix Fist",
                "Phoenix King", "Phoenix Knight", "Phoenix Panther", "Phoenix Widow", "Pink Bubble", "Powder Pink",
                "Presidential Attire", "Punisher 2099", "Punk Rage", "Queen of Diamonds", "Queen of Wakanda", "Red Panda",
                "Red Runway Veil", "Restful Recovery", "Ronin", "Revolution", "Robe of Relaxation", "Rocket of the Rafters",
                "Rocky Tide", "Rogue Redux", "Seat of Autumn", "Seat of Spring", "Shining Star", "Sorcerer Supreme of the Galaxy",
                "Space Knight", "Star Spangled Style", "Starlit Outlaw", "Steam Power", "Sunny Sizzler", "Sunshine Squirrel",
                "Sunshine Thing", "Superior Iron Man", "Sword Master", "Symbiote Raccoon", "Symbiote Storm", "Symbiote-Thing",
                "Temporal Tyrant", "The Fantastic Four: First Steps", "The Maker", "The Queen Of The Dead", "The Trial of Magneto", "Thieves Guildmaster",
                "Thrice-Cursed King", "Thunderbolts*", "Tidal Trickery", "Trench Coat", "Twilight Duo", "Ultimate Wind-Rider",
                "Urban Hunter", "VEN#M", "Vengeance", "Verdant Vanguard", "Wasteland Mech", "Wasteland Robot",
                "Weapon PhoeniX", "Weapon X", "White Suit", "Winter Buckaroo", "Winter Soldier", "Winter's Wrath",
                "Worthy Waves", "X-Revolution", "X-Tron", "Yami no Karasu", "Yatsukahagi", "Young Blood"
            };
            for (auto s : epic) r[s] = SR_EPIC;
            const char* legendary[] = {
                "Abyssal Glow", "All-Butcher", "Alluring Apple", "Amber Annihilator", "Amethyst Aura", "Aqua Arsenal",
                "Avengers: Endgame", "Avengers: Infinity War", "Azure Shade", "Big Shot", "Binary Sword", "Bleeker Street Strut",
                "Blood Berserker", "Blue Barrage", "Blue Bolt", "Blue Breeze", "Bluebell Breeze", "Blueberry Ice",
                "Boogie Bolt", "Business Shark", "Capwolf", "Chaos Phoenix", "Cherry Delight", "Citrus Sunrise",
                "Cool Summer", "Cosmic Warlock", "Damisa-Sarki", "Dance Partner", "Dangan Ronin", "Daring Daifuku",
                "Deep Green Magic", "Devouring Duo", "Disappearing Dessert", "Disco Pop", "Doctor Strange in the Multiverse of Madness", "Duck Defender",
                "Emerald Executioner", "Extrembiote Armor", "Floral Frights", "Frostbitten Witch", "Frosted Agony", "Fruit Cake Flurry",
                "Galactic Wings", "Goddess of Thunder", "Grape Wagashi", "Green Bill", "Green Scar", "Gummy Surprise",
                "Hellfire Protocol", "Ice Pas de Deux", "Ice Phoenix", "Immortal Sovereign", "Iron Mariner", "Jade Jewel",
                "Jeff O'lantern", "Lady Loki", "Lion's Gaze", "Lively Lion", "Lunar General", "Lush Luminance",
                "Majestic Mauve", "Majestic Raiment", "Mango Magic", "Matcha Mirage", "Mecha-Flora", "Mirae 2099",
                "Monstro King", "Moon Knight Mech", "Moonlit Mirage", "Mr. X", "Mrs. X", "Neon Nimbus",
                "New Millennia Might", "Night Nebula", "Oceanic Harmony", "Orange Edge", "Phage Palette", "Phantom Tide",
                "Phoenix Chaos", "Plasma Pulse", "Polarity Bond", "Polarity Edge", "Polarity Soldier", "Prism Parade",
                "Prismatic Pulse", "Psychedelic Pulse", "Punkchild", "Purple Pulse", "Queen in Black", "Radiant Radiance",
                "Radiant Ray", "Radiant Reef", "Reborn King in Black", "Rosy Resilience", "Savage Spirit",
                "Shadow Shock", "Shin Sagi-Shi", "Skeleton", "Skuld 2099", "Snow Symbiote", "Snow-SP//DR",
                "Sorcerer Immortal", "Space Corsair", "Spider-Man: No Way Home", "Spider-Oni", "Spider-Punk 2099", "Starcracker",
                "Sunshine Land Shark", "Sunshine Raccoon", "Symbiote Squirrel", "Tangerine Tint", "The Grim Lady", "The Life Fantastic",
                "Thor: Love and Thunder", "Tinsel Tail", "Turbo Tailwind", "Twilight Tones", "Twisted Conjurer", "Verdant Vortex",
                "Violet Veil", "Vivid Vibe", "White Waddle", "Witch of the Evil Eye", "Yggroot"
            };
            for (auto s : legendary) r[s] = SR_LEGENDARY;
            return r;
        }();
        auto it = m.find(name);
        return it != m.end() ? it->second : SR_COMMON;
    }

    inline ImVec4 GetRarityColor(SkinRarity r) {
        switch (r) {
        case SR_LEGENDARY: return ImVec4(1.00f, 0.84f, 0.00f, 1.00f);
        case SR_EPIC:      return ImVec4(0.64f, 0.21f, 0.93f, 1.00f);
        case SR_RARE:      return ImVec4(0.30f, 0.55f, 1.00f, 1.00f);
        default:           return ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        }
    }

    inline ImU32 GetRarityColorU32(SkinRarity r) {
        switch (r) {
        case SR_LEGENDARY: return IM_COL32(255, 214, 0, 255);
        case SR_EPIC:      return IM_COL32(163, 53, 238, 255);
        case SR_RARE:      return IM_COL32(77, 140, 255, 255);
        default:           return IM_COL32(166, 166, 166, 255);
        }
    }

    inline const char* GetRarityName(SkinRarity r) {
        switch (r) {
        case SR_LEGENDARY: return "Legendary";
        case SR_EPIC:      return "Epic";
        case SR_RARE:      return "Rare";
        default:           return "Common";
        }
    }

    inline std::vector<std::pair<int32_t, std::string>> GetSortedHeroes() {
        std::vector<std::pair<int32_t, std::string>> heroes(HeroNames.begin(), HeroNames.end());
        std::sort(heroes.begin(), heroes.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
        return heroes;
    }

    inline std::vector<std::pair<int32_t, std::string>> GetSortedSkins(int32_t heroId) {
        std::vector<std::pair<int32_t, std::string>> skins;
        auto it = SkinNames.find(heroId);
        if (it != SkinNames.end()) {
            skins.assign(it->second.begin(), it->second.end());
            std::sort(skins.begin(), skins.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        }
        return skins;
    }
}
