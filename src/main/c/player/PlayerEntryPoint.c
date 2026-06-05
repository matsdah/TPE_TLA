#include <raylib.h>
#include <stdio.h>

const int main(const int length, const char ** arguments) {
	printf("Flex-Bison-Player placeholder starting...\n");

	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Flex-Bison-Player");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Flex-Bison-Player placeholder", 190, 200, 20, LIGHTGRAY);
		EndDrawing();
	}

	CloseWindow();
	printf("Flex-Bison-Player placeholder exiting.\n");
	return 0;
}
