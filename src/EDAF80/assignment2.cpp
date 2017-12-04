#include "assignment2.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment2::~Assignment2()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment2::run()
{
    auto move_bool = true;
    
	// Load the sphere geometry
	auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
    auto const sphere = parametric_shapes::createSphere(40u, 40u, 1.0f);
	if (shape.vao == 0u)
		return;

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
	if (diffuse_shader == 0u) {
		LogError("Failed to load diffuse shader");
		return;
	}
	auto normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
	if (normal_shader == 0u) {
		LogError("Failed to load normal shader");
		return;
	}
	auto texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
	if (texcoord_shader == 0u) {
		LogError("Failed to load texcoord shader");
		return;
	}

	auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};
    
    // set up control points
    auto size = 5;
    auto controlPoints  = std::vector<glm::vec3>(5);
    
    for(int i=0; i<controlPoints.size(); i++)
    {
        controlPoints[i] = glm::vec3(rand()%size, rand()%size, rand()%size);
    }

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.0f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;
    
    bool second_sphere = false;

	auto circle_rings = Node();
	circle_rings.set_geometry(shape);
	circle_rings.set_program(fallback_shader, set_uniforms);
    
    auto sphereTest = Node();
    sphereTest.set_geometry(sphere);
    sphereTest.set_program(fallback_shader, set_uniforms);
    
    auto sphere2 = Node();
    sphere2.set_geometry(sphere);
    sphere2.set_program(fallback_shader, set_uniforms);

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
//	glEnable(GL_CULL_FACE);
//	glCullFace(GL_FRONT);
//	glCullFace(GL_BACK);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;
    
    int i=1, j=2, k=3, l=0;
    int tempTime=0;
    glm::vec3 currentLocation, currentLocation2;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();


		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			circle_rings.set_program(fallback_shader, set_uniforms);
            sphereTest.set_program(fallback_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			circle_rings.set_program(diffuse_shader, set_uniforms);
            sphereTest.set_program(diffuse_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			circle_rings.set_program(normal_shader, set_uniforms);
            sphereTest.set_program(normal_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			circle_rings.set_program(texcoord_shader, set_uniforms);
            sphereTest.set_program(texcoord_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		switch (polygon_mode) {
			case polygon_mode_t::fill:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case polygon_mode_t::line:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case polygon_mode_t::point:
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				break;
		}

		//! \todo Interpolate the movement of a shape between various
		//!        control points
        
        
        if (j > controlPoints.size() - 1)
        {
            j = 0;
        }
        
        if (k > controlPoints.size() - 1)
        {
            k = 0;
        }
        
        if (l > controlPoints.size() - 1)
        {
            l = 0;
        }
        
        if(i > controlPoints.size()-1)
        {
            i = 0;
        }
        
        if(use_linear)
        {
            currentLocation = interpolation::evalLERP(controlPoints[i], controlPoints[j], nowTime-tempTime);
            currentLocation2 = interpolation::evalLERP(controlPoints[i], controlPoints[j], nowTime-tempTime);
        }
        else
        {
            currentLocation = interpolation::evalCatmullRom(controlPoints[l], controlPoints[i], controlPoints[j], controlPoints[k],catmull_rom_tension, nowTime - tempTime);
            currentLocation2 = interpolation::evalCatmullRom(controlPoints[l], controlPoints[i], controlPoints[j], controlPoints[k],catmull_rom_tension/2, nowTime - tempTime);
        }
        
        if(move_bool)
        {
            circle_rings.rotate_y(0.01f);
            sphereTest.set_translation(currentLocation);
            sphere2.set_translation(currentLocation2);
        }
        
        if(nowTime-tempTime > 1)
        {
            tempTime = nowTime;
            i++;
            j++;
            k++;
            l++;
        }

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		circle_rings.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());
        sphereTest.render(mCamera.GetWorldToClipMatrix(), sphereTest.get_transform());
        
        if(second_sphere)
        {
            sphere2.render(mCamera.GetWorldToClipMatrix(), sphere2.get_transform());
        }

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
            ImGui::Checkbox("Second sphere at half tension", &second_sphere);
            ImGui::Checkbox("Move", &move_bool);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texcoord_shader);
	normal_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
