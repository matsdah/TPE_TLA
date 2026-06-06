// Test story with real assets
// Compile and run to verify media loading works

character "Hero" as hero color #FF5733;

asset bg_main = "../src/test/assets/img/background.png";
asset char_sprite = "../src/test/assets/img/character.png";
asset sfx_beep = "../src/test/assets/audio/effect.wav";
asset bgm_music = "../src/test/assets/audio/music.wav";

scene "Test" {
    show background bg_main;
    show sprite char_sprite;
    play music bgm_music;
    play sound sfx_beep;
    hero "Assets loaded successfully!";
    hide sprite char_sprite;
    stop sound sfx_beep;
    end;
}
