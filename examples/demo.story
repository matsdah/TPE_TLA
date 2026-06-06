// Technical showcase demonstrating every language construct.
// Compile: src/main/bash/run.sh examples/demo.story
// Play:    src/main/bash/play.sh

character "Narrator" as narr color #AAAAAA;
character "Aria"     as aria color #FF5733;
character "Guardian" as guard color #33AAFF;

asset bg_title  = "../src/test/assets/img/background.png";
asset spr_aria  = "../src/test/assets/img/character.png";
asset bgm_main  = "../src/test/assets/audio/music.wav";
asset sfx_win   = "../src/test/assets/audio/effect.wav";

set score = 0;
set bonus = 2;

scene "Start" {
    show background bg_title;
    play music bgm_main;
    narr "Welcome to the technical showcase.";
    show sprite spr_aria;
    aria "Every DSL feature will be exercised in sequence.";
    narr "Make a choice to mutate state variables.";
    choice {
        "Add bonus" {
            set score += bonus;
            aria "Score increased by bonus.";
            goto "Check";
        }
        "Subtract penalty" {
            set score -= 5;
            aria "Score decreased by penalty.";
            goto "Check";
        }
        "Double and add" {
            set score = (bonus * 3) + 1;
            aria "Score set via arithmetic expression.";
            goto "Check";
        }
    }
}

scene "Check" {
    guard "Evaluating condition...";
    if (score > 5) {
        guard "High score detected.";
        goto "Win";
    } else {
        guard "Score too low.";
        goto "Lose";
    }
}

scene "Win" {
    aria "Congratulations! You triggered the win branch.";
    play sound sfx_win;
    narr "This demonstrates play sound, stop sound, and stop music.";
    stop sound sfx_win;
    hide sprite spr_aria;
    stop music bgm_main;
    end;
}

scene "Lose" {
    guard "You reached the lose branch.";
    hide sprite spr_aria;
    hide background bg_title;
    stop music bgm_main;
    narr "Showcase complete. End statement reached.";
    end;
}
