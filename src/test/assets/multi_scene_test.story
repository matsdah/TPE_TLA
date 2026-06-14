// --- Declaraciones de personajes ---
character "Narrador"  as narr  color #FFFFFF;
character "Aria"      as aria  color #FF5733;
character "Guía"      as guide color #00FF88;
character "Villano"   as villain color #AA0000;

// --- Assets ---
asset bg_forest    = "../src/test/assets/img/forest.png";
asset bg_cave      = "../src/test/assets/img/cave.png";
asset spr_aria     = "../src/test/assets/img/aria.png";
asset spr_guide    = "../src/test/assets/img/guide.png";
asset spr_villain  = "../src/test/assets/img/villain.png";
asset bgm_main     = "../src/test/assets/audio/music.wav";
asset sfx_effect   = "../src/test/assets/audio/effect.wav";

// --- Variables globales ---
set affinity  = 0;
set courage   = 10;
set items     = 0;
set loops     = 0;

// =============================================================
// ESCENA 1: Introducción lineal (bosque + Aria + música)
// =============================================================
scene "Intro" {
    show background bg_forest;
    show sprite spr_aria;
    play music bgm_main;
    narr "El viento sopla frío entre los árboles.";
    aria "No deberíamos estar aquí...";
    narr "De repente, una rama cruje detrás de ellos.";
    goto "Meet_Guide";
}

// =============================================================
// ESCENA 2: Aparece el guía (sprite guide) con loop cíclico
// =============================================================
scene "Meet_Guide" {
    show sprite spr_guide;
    guide "Soy el guardián del bosque. Puedo orientarlos, pero confíen en mí.";
    set loops += 1;
    choice {
        "Confiar en el guía" {
            set affinity += 3;
            guide "Sabia decisión. Síganme hacia la cueva.";
            goto "First_Choice";
        }
        "Hacer otra pregunta" {
            if (loops > 2) {
                guide "Ya no hay más tiempo. Deben avanzar.";
                goto "First_Choice";
            } else {
                guide "Pregunten lo que necesiten.";
                goto "Meet_Guide";
            }
        }
        "Desconfiar" {
            set affinity -= 2;
            guide "Como gusten, pero el peligro no espera.";
            goto "First_Choice";
        }
    }
}

// =============================================================
// ESCENA 3: Primera bifurcación (Aria de vuelta)
// =============================================================
scene "First_Choice" {
    show sprite spr_aria;
    aria "¿Qué hacemos ahora?";
    choice {
        "Investigar el ruido" {
            set courage += 5;
            aria "¡Vamos a ver qué fue!";
            goto "Investigate";
        }
        "Buscar un refugio" {
            set courage -= 3;
            aria "Mejor escondámonos un momento...";
            goto "Investigate";
        }
    }
}

// =============================================================
// ESCENA 4: Investigar → condicional sobre courage
// =============================================================
scene "Investigate" {
    play sound sfx_effect;
    narr "Se adentran en la espesura siguiendo el sonido.";
    if (courage > 12) {
        aria "¡Tengo el valor para enfrentar lo que sea!";
        set items += 1;
        goto "Find_Item";
    } else {
        aria "Tengo miedo, pero sigamos...";
        goto "Find_Item";
    }
}

// =============================================================
// ESCENA 5: Encontrar ítem → if anidado
// =============================================================
scene "Find_Item" {
    narr "Encuentran una antorcha encendida en el suelo.";
    if (items > 0) {
        if (affinity > 0) {
            guide "La hallaron gracias a la confianza mutua. Tómenla.";
        } else {
            narr "La toman sin dudar. Les servirá en la oscuridad.";
        }
        set items += 1;
    } else {
        narr "Dudan, pero finalmente la recogen.";
        set items += 1;
    }
    goto "Cave_Entrance";
}

// =============================================================
// ESCENA 6: Entrada a la cueva (bg_cave) — camino obligado
// =============================================================
scene "Cave_Entrance" {
    stop music bgm_main;
    show background bg_cave;
    hide sprite spr_guide;
    play sound sfx_effect;
    narr "La antorcha ilumina la entrada de una cueva profunda.";
    aria "Algo se mueve en las sombras...";
    goto "Encounter_Villain";
}

// =============================================================
// ESCENA 7: Encuentro con el villano (spr_villain) en la cueva
// =============================================================
scene "Encounter_Villain" {
    hide sprite spr_aria;
    show sprite spr_villain;
    villain "¡Nadie escapa de mi cueva!";
    choice {
        "Enfrentarlo" {
            if (courage > 8) {
                villain "Impresionante... son dignos. Les dejo pasar.";
                set items += 5;
                goto "Resolution";
            } else {
                villain "¡Demasiado débiles!";
                goto "Bad_End";
            }
        }
        "Negociar con calma" {
            set affinity += 1;
            villain "Hmm... está bien. Esta vez pasarán.";
            goto "Resolution";
        }
    }
}

// =============================================================
// ESCENA 8: Resolución → vuelta al bosque (aritmética acumulativa)
// =============================================================
scene "Resolution" {
    stop sound sfx_effect;
    show background bg_forest;
    show sprite spr_aria;
    play music bgm_main;
    narr "Salen de la cueva hacia la luz del bosque.";
    aria "Lo logramos juntos.";
    set items += courage;
    set items -= 3;
    if (items > 10) {
        goto "Good_End";
    } else {
        goto "Neutral_End";
    }
}

// =============================================================
// FINALES
// =============================================================
scene "Good_End" {
    show sprite spr_guide;
    stop music bgm_main;
    narr "¡Superaron todos los obstáculos con valentía!";
    aria "Lo logramos.";
    guide "Fue un honor acompañarlos.";
    end;
}

scene "Neutral_End" {
    stop music bgm_main;
    narr "La aventura termina de manera agridulce.";
    aria "Podría haber sido peor.";
    end;
}

scene "Bad_End" {
    show background bg_cave;
    show sprite spr_villain;
    stop music bgm_main;
    stop sound sfx_effect;
    villain "El bosque los reclama.";
    narr "Nadie volvió a verlos.";
    end;
}
