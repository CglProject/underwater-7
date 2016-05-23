#include "RenderProject.h"

/* Initialize the Project */
void RenderProject::init()
{
	bRenderer::loadConfigFile("config.json");	// load custom configurations replacing the default values in Configuration.cpp

	// let the renderer create an OpenGL context and the main window
	if(Input::isTouchDevice())
		bRenderer().initRenderer(true);										// full screen on iOS
	else
	{

	}
		bRenderer().initRenderer(1200, 700, false, "Underwater Game");		// windowed mode on desktop
		//bRenderer().initRenderer(View::getScreenWidth(), View::getScreenHeight(), true);		// full screen using full width and height of the screen

	// start main loop 
	bRenderer().runRenderer();
}

/* This function is executed when initializing the renderer */
void RenderProject::initFunction()
{
	// get OpenGL and shading language version
	bRenderer::log("OpenGL Version: ", glGetString(GL_VERSION));
	bRenderer::log("Shading Language Version: ", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// initialize variables
	_offset = 0.0f;
	_randomOffset = 0.0f;
	_cameraSpeed = 40.0f;
	_running = false; _lastStateSpaceKey = bRenderer::INPUT_UNDEFINED;
	_viewMatrixHUD = Camera::lookAt(vmml::Vector3f(0.0f, 0.0f, 0.25f), vmml::Vector3f::ZERO, vmml::Vector3f::UP);
    
    
    
    
    // Part of game logic: specify target positions
    targetPositions.push_back(vmml::Vector3f(31.520990, -70.115227, -325.600250));
    targetRadiuses.push_back(100.0);
    
    
    
    
	// set shader versions (optional)
	bRenderer().getObjects()->setShaderVersionDesktop("#version 120");
	bRenderer().getObjects()->setShaderVersionES("#version 100");



	// create camera
	bRenderer().getObjects()->createCamera("camera", vmml::Vector3f(-33.0, 0.f, -13.0), vmml::Vector3f(0.f, -M_PI_F / 2, 0.f));

	// create light
	bRenderer().getObjects()->createLight("light", vmml::Vector3f(0,600,0) , vmml::Vector3f(1, 1, 1), 10000.0f, 0.0f, 10000.0f);
	vmml::Vector3f cameraPosition = -bRenderer().getObjects()->getCamera("camera")->getPosition();
	cameraPosition.operator+=(vmml::Vector3f(0, 20, 0));
	bRenderer().getObjects()->createLight("cameraLight", cameraPosition, vmml::Vector3f(1, 1, 1), 1000.0f, 0.1f, 200.0f);
    
    // Load textures for caustics
    causticsCount = 32;
    for (int i = 0; i < causticsCount; ++i) {
        bRenderer().getObjects()->loadTexture(std::string("caust") + (i < 10 ? "0" : "") + std::to_string(i) + ".png");
    }
    
    bRenderer().getObjects()->createSprite("menubackscreen", "menubackscreen.png");
    
    // Load shaders
    bRenderer().getObjects()->loadShaderFile("Plane");
    
	// load models
	bRenderer().getObjects()->loadObjModel("ground.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("Rock1.obj", true, true, false, 2, true, false);
    
	// bRenderer().getObjects()->loadObjModel("Manta0.obj", true, true, false, 0, true, false);
    
	bRenderer().getObjects()->loadObjModel("zahnrad-neu.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("seats.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("aquaplant1-r.obj", true, true, false, 2, true, false);
	//bRenderer().getObjects()->loadObjModel("aquaplant2-r.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("aquaplant4.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("aquaplant5.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("suitcase.obj", true, true, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("wing.obj", true, false, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("heck.obj", true, false, false, 2, true, false);
	bRenderer().getObjects()->loadObjModel("engine.obj", true, false, false, 2, true, false);



	// sprites
	// bRenderer().getObjects()->createSprite("groundSprite", "sand.jpg");


	/* Begin postprocessing stuff */
	bRenderer().getObjects()->createFramebuffer("fbo");					// create framebuffer object
	bRenderer().getObjects()->createTexture("fbo_texture1", 0.f, 0.f);	// create texture to bind to the fbo

	bRenderer().getObjects()->createFramebuffer("fogFbo");
	bRenderer().getObjects()->createTexture("fogFbo_texture", 0.f, 0.f);
	DepthMapPtr dmapp = bRenderer().getObjects()->createDepthMap("dmapp", bRenderer().getView()->getWidth(), bRenderer().getView()->getHeight());

	ShaderPtr fogShader = bRenderer().getObjects()->loadShaderFile("fogShader", 0, false, false, false, false, false);
	MaterialPtr fogMaterial = bRenderer().getObjects()->createMaterial("fogMaterial", fogShader);
	bRenderer().getObjects()->createSprite("fogSprite", fogMaterial);
	/* END postprocessing stuff */
    
    
    
    
    
    // Set game to initial state
    resetGame();
    
    
    


	// Update render queue
	updateRenderQueue("camera", 0.0f);
}

void RenderProject::resetGame() {
    // No targets have been found yet.
    targetFound.clear();
    for (int i = 0; i < targetPositions.size(); ++i) {
        targetFound.push_back(false);
    }
    targetsLeft = (int) targetPositions.size();
    bRenderer::log(std::string("Game reset, targets left: ") + std::to_string(targetsLeft));
    
    // Move player back to starting point
    CameraPtr camera = bRenderer().getObjects()->getCamera("camera");
    camera->setPosition(vmml::Vector3f(-33.0, 0.f, -13.0));
    camera->setRotation(vmml::Vector3f(0.f, -M_PI_F / 2, 0.f));
    
    // Reset available time
    timeLeft = 10.0;
    
    won = false;
    lost = false;
}

/* Draw your scene here */
void RenderProject::loopFunction(const double &deltaTime, const double &elapsedTime)
{
//	bRenderer::log("FPS: " + std::to_string(1 / deltaTime));	// write number of frames per second to the console every
    
    if (_running) {
        // Check if targets were found
        vmml::Vector3f playerPosition = -bRenderer().getObjects()->getCamera("camera")->getPosition();
        for (int i = 0; i < targetPositions.size(); ++i) {
            if (!targetFound.at(i)) {
                float distance = (targetPositions.at(i) - playerPosition).length();
                bRenderer::log(std::string("dist: ") + std::to_string(distance) + ", the radius: " + std::to_string(targetRadiuses.at(i)));
                if (distance <= targetRadiuses.at(i)) {
                    targetFound.at(i) = true;
                    --targetsLeft;
                }
            }
        }
        
        
        timeLeft -= deltaTime;
        
        if (targetsLeft == 0) { // Won?
            won = true;
            _running = false;
        } else if (timeLeft <= 0) { // Lost?
            lost = true;
            _running = false;
        }
    }
    

    //// Draw Scene and do post processing ////
    
    // Determine current cuastics texture index
    causticsIndex = floor(fmod ((float) elapsedTime / causticsPeriod, (float) causticsCount));
    
    /* Begin postprocessing */
    
    // bind depth map and render the scene
    GLint defaultFBO;
    defaultFBO = Framebuffer::getCurrentFramebuffer();

    bRenderer().getObjects()->getFramebuffer("fogFbo")->bindDepthMap(bRenderer().getObjects()->getDepthMap("dmapp"), true);

    bRenderer().getModelRenderer()->drawQueue(/*GL_LINES*/);

    // bind the texture and render the scene again
    bRenderer().getObjects()->getFramebuffer("fbo")->bindTexture(bRenderer().getObjects()->getTexture("fogFbo_texture"), true);
    bRenderer().getModelRenderer()->drawQueue(/*GL_LINES*/);
    bRenderer().getModelRenderer()->clearQueue();

    bRenderer().getObjects()->getFramebuffer("fogFbo")->unbind(defaultFBO);

    bRenderer().getObjects()->getShader("fogShader")->setUniform("depthmap_texture", bRenderer().getObjects()->getDepthMap("dmapp"));
    bRenderer().getObjects()->getShader("fogShader")->setUniform("fogFbo_texture", bRenderer().getObjects()->getTexture("fogFbo_texture"));
    float passedTime = float(elapsedTime);
    //passedTime =  modf(passedTime, M_PI);
    bRenderer().getObjects()->getShader("fogShader")->setUniform("passedTime", passedTime);	

    vmml::Matrix4f modelMatrix = vmml::create_translation(vmml::Vector3f(0.0f, 0.0f, -0.5));


    bRenderer().getModelRenderer()->drawModel(bRenderer().getObjects()->getModel("fogSprite"), modelMatrix, _viewMatrixHUD, vmml::Matrix4f::IDENTITY, std::vector<std::string>({}), false);
    
    /*modelMatrix = vmml::create_translation(vmml::Vector3f(0.0f, 0.0f, -0.6));
    
    bRenderer().getModelRenderer()->drawModel(bRenderer().getObjects()->getModel("menubackscreen"), modelMatrix, _viewMatrixHUD, vmml::Matrix4f::IDENTITY, std::vector<std::string>({}), false);*/

    /* End postprocessing */
    
    GLfloat titleScale = 0.5f;
    vmml::Matrix4f scaling = vmml::create_scaling(vmml::Vector3f(titleScale / bRenderer().getView()->getAspectRatio(), titleScale, titleScale));
    modelMatrix = vmml::create_translation(vmml::Vector3f(-0.0f, 0.0f, -0.65f)) * scaling;
    // draw
    bRenderer().getModelRenderer()->drawModel(bRenderer().getObjects()->getModel("menubackscreen"), modelMatrix, _viewMatrixHUD, vmml::Matrix4f::IDENTITY, std::vector<std::string>({}), false, false);
    
    vmml::Vector3f currentPosition = bRenderer().getObjects()->getCamera("camera")->getPosition();
    std::string x = std::to_string(-currentPosition.array[0]);
    std::string y = std::to_string(-currentPosition.array[1]);
    std::string z = std::to_string(-currentPosition.array[2]);
    bRenderer::log("Current Position: " + x + " / " + y + " / " + z);


    //// Camera Movement ////
    updateCamera("camera", deltaTime);

    // light position moving with camera
    vmml::Vector3f cameraPosition = -bRenderer().getObjects()->getCamera("camera")->getPosition();
    cameraPosition.operator+=(vmml::Vector3f(3,0,0));
    bRenderer().getObjects()->getLight("cameraLight")->setPosition(cameraPosition);

    /// Update render queue ///
    updateRenderQueue("camera", deltaTime);
    

	// Quit renderer when escape is pressed
	if (bRenderer().getInput()->getKeyState(bRenderer::KEY_ESCAPE) == bRenderer::INPUT_PRESS)
		bRenderer().terminateRenderer();
}

/* This function is executed when terminating the renderer */
void RenderProject::terminateFunction()
{
	bRenderer::log("I totally terminated this Renderer :-)");
}

/* Update render queue */
void RenderProject::updateRenderQueue(const std::string &camera, const double &deltaTime)
{
    // Get the texture for the caustics (loaded in the init function)
    TexturePtr causticsTexture = bRenderer().getObjects()->getTexture(std::string("caust") + (causticsIndex < 10 ? "0" : "") + std::to_string(causticsIndex));
    
    // About the order of rotations and translations: note that a *= b is the same as a = a * b and not a = b * a
    
	// draw Ground
    ShaderPtr floorShader = bRenderer().getObjects()->getShader("Plane");
    vmml::Matrix4f modelMatrix1 = vmml::create_translation(vmml::Vector3f(0.0f, -100.0f, -10.0f)) * vmml::create_scaling(vmml::Vector3f(10.0f));
    modelMatrix1 *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
    vmml::Matrix3f normalMatrix1;
    vmml::compute_inverse(vmml::transpose(vmml::Matrix3f(modelMatrix1)), normalMatrix1);
    floorShader->setUniform("ModelMatrix", modelMatrix1);
    floorShader->setUniform("NormalMatrix", normalMatrix1);
    floorShader->setUniform("CausticsTexture", causticsTexture);
	//bRenderer().getModelRenderer()->drawModel("plane", camera, modelMatrix6, std::vector<std::string>({ "testLight"}), true, false);
	//bRenderer().getModelRenderer()->drawModel("groundModel", camera, modelMatrix6, std::vector<std::string>({ "testLight"}), true, false);
	bRenderer().getModelRenderer()->queueModelInstance("ground", "ground_instance", camera, modelMatrix1, std::vector<std::string>({ "light", "cameraLight"}));

	vmml::Matrix4f modelMatrix = vmml::create_translation(vmml::Vector3f(320.0f, -130.0f, 0.0f)) * vmml::create_scaling(vmml::Vector3f(50.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("Rock1", "rock_instance1", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-320.0f, -110.0f, 300.0f)) * vmml::create_scaling(vmml::Vector3f(3.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F-0.4f, vmml::Vector3f(0.0f, 6.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("Rock1", "rock_instance2", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-320.0f, -15.0f, -450.0f)) * vmml::create_scaling(vmml::Vector3f(5.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("Rock1", "rock_instance3", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));




	//modelMatrix3 *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.0f , 0.f));
	//modelMatrix3 *= vmml::create_translation(vmml::Vector3f(-100.0f, 0.f, 100.0f));
	//bRenderer().getModelRenderer()->queueModelInstance("Rock1", "rock2_instance", camera, modelMatrix3, std::vector<std::string>({}));

	modelMatrix = vmml::create_translation(vmml::Vector3f(150.0f, -13.8f, 320.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("zahnrad-neu", "zahnrad_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	// ground Sprite
	//vmml::Matrix4f modelMatrix5 = vmml::create_translation(vmml::Vector3f(0.0f, -100.0f, -10.0f)) * vmml::create_scaling(vmml::Vector3f(50.0f));
	//modelMatrix5 *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(1.0f, 0.f, 0.f));
	//bRenderer().getModelRenderer()->drawModel("groundSprite", camera, modelMatrix5, std::vector<std::string>({}), true, false);

	modelMatrix = vmml::create_translation(vmml::Vector3f(450.0f, -7.0f, -400.0f)) * vmml::create_scaling(vmml::Vector3f(10.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(1.0f, 0.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("seats", "seats_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-285.0f, -90.5f, -230.0f)) * vmml::create_scaling(vmml::Vector3f(0.4f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F-0.4f, vmml::Vector3f(1.0f, 0.f, 0.f));
	modelMatrix *= vmml::create_rotation(-0.6f, vmml::Vector3f(0.0f, 1.f, 0.f));
	modelMatrix *= vmml::create_rotation(0.3f, vmml::Vector3f(0.0f, 0.f, 1.f));
	bRenderer().getModelRenderer()->queueModelInstance("suitcase", "suitcase_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	
    
    
    
    
    modelMatrix = vmml::create_translation(vmml::Vector3f(120.0f, -30.5f, -430.0f)) * vmml::create_scaling(vmml::Vector3f(0.7f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F+0.3f, vmml::Vector3f(1.0f, 0.f, 0.f));
	modelMatrix *= vmml::create_rotation(M_PI_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	modelMatrix *= vmml::create_rotation(1.3f, vmml::Vector3f(0.0f, 0.f, 1.f));
	bRenderer().getModelRenderer()->queueModelInstance("wing", "wing_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));
    
    

	
    
    
    
    
    
    modelMatrix = vmml::create_translation(vmml::Vector3f(-320.0f, -50.0f, 350.0f)) * vmml::create_scaling(vmml::Vector3f(0.7f));
	//modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(1.0f, 0.f, 0.f));
	modelMatrix *= vmml::create_rotation(M_PI_F+0.4f, vmml::Vector3f(0.0f, 0.f, 1.f));
	bRenderer().getModelRenderer()->queueModelInstance("heck", "heck_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(500.0f, 80.0f, 315.0f)) * vmml::create_scaling(vmml::Vector3f(0.7f));
	modelMatrix *= vmml::create_rotation(0.6f, vmml::Vector3f(0.0f, 0.f, 1.f));
	bRenderer().getModelRenderer()->queueModelInstance("engine", "engine_instance", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));



	//// Plants
	modelMatrix = vmml::create_translation(vmml::Vector3f(300.0f, -60.5f, -80.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance1", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(300.0f, -60.5f, -130.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	//modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 4.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance2", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(350.0f, -60.5f, -110.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance3", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	/*modelMatrix = vmml::create_translation(vmml::Vector3f(-280.0f, 8.0f, -460.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance4", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-280.0f, -79.3f, 460.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance4", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(280.0f, 21.5f, 310.0f)) * vmml::create_scaling(vmml::Vector3f(0.8f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance4", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-360.0f, -85.0f, -176.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance5", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));
     */
	// modelMatrix = vmml::create_translation(vmml::Vector3f(-353.0f, -85.0f, -210.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
    modelMatrix = vmml::create_translation(vmml::Vector3f(31.520990, -70.115227, -325.600250)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance6", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-337.0f, -85.0f, -299.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance7", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-280.0f, -85.0f, -230.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance8", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-215.0f, 59.0f, 63.0f)) * vmml::create_scaling(vmml::Vector3f(0.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance9", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(130.0f, -80.5f, -80.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance10", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(100.0f, -80.5f, -80.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance10", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(469.0f, -0.5f, -440.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance11", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(468.0f, -0.5f, -360.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance12", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-81.0f, -3.5f, 277.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance13", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(416.0f, -3.5f, -402.0f)) * vmml::create_scaling(vmml::Vector3f(0.70f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance13", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-353.0f, -78.5f, 349.0f)) * vmml::create_scaling(vmml::Vector3f(1.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance13", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(390.0f, -73.5f, 446.0f)) * vmml::create_scaling(vmml::Vector3f(1.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance14", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(-47.0f, 46.5f, 219.0f)) * vmml::create_scaling(vmml::Vector3f(1.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant4", "plant_instance15", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	/*modelMatrix = vmml::create_translation(vmml::Vector3f(410.0f, -35.5f, -52.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance16", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));*/

	modelMatrix = vmml::create_translation(vmml::Vector3f(440.0f, 25.0f, 200.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant1-r", "plant_instance17", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	/*modelMatrix = vmml::create_translation(vmml::Vector3f(417.0f, 50.5f, -363.0f)) * vmml::create_scaling(vmml::Vector3f(1.5f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant2-r", "plant_instance18", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));*/

	modelMatrix = vmml::create_translation(vmml::Vector3f(417.0f, 50.5f, -363.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant5", "plant_instance19", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));

	modelMatrix = vmml::create_translation(vmml::Vector3f(417.0f, 50.5f, -363.0f)) * vmml::create_scaling(vmml::Vector3f(1.0f));
	modelMatrix *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
	bRenderer().getModelRenderer()->queueModelInstance("aquaplant5", "plant_instance20", camera, modelMatrix, std::vector<std::string>({ "light", "cameraLight" }));


	// -186.531738 / 72.730522 / 44.357285
	// -459.170380 / -1.045390 / 4.113952
	// 417.463348 / 0.422991 / -363.250793

}

/* Camera movement */
void RenderProject::updateCamera(const std::string &camera, const double &deltaTime)
{
	//// Adjust aspect ratio ////
	bRenderer().getObjects()->getCamera(camera)->setAspectRatio(bRenderer().getView()->getAspectRatio());

	double deltaCameraY = 0.0;
	double deltaCameraX = 0.0;
	double cameraForward = 0.0;
	double cameraSideward = 0.0;

	/* iOS: control movement using touch screen */
	if (Input::isTouchDevice()){

		// pause using double tap
		if (bRenderer().getInput()->doubleTapRecognized()){
			_running = !_running;
		}

		if (_running){
			// control using touch
			TouchMap touchMap = bRenderer().getInput()->getTouches();
			int i = 0;
			for (auto t = touchMap.begin(); t != touchMap.end(); ++t)
			{
				Touch touch = t->second;
				// If touch is in left half of the view: move around
				if (touch.startPositionX < bRenderer().getView()->getWidth() / 2){
					cameraForward = -(touch.currentPositionY - touch.startPositionY) / 100;
					cameraSideward = (touch.currentPositionX - touch.startPositionX) / 100;

				}
				// if touch is in right half of the view: look around
				else
				{
					deltaCameraY = (touch.currentPositionX - touch.startPositionX) / 2000;
					deltaCameraX = (touch.currentPositionY - touch.startPositionY) / 2000;
				}
				if (++i > 2)
					break;
			}
		}

	}
	/* Windows: control movement using mouse and keyboard */
	else{
		// use space to pause and unpause
		GLint currentStateSpaceKey = bRenderer().getInput()->getKeyState(bRenderer::KEY_SPACE);
		if (currentStateSpaceKey != _lastStateSpaceKey)
		{
			_lastStateSpaceKey = currentStateSpaceKey;
			if (currentStateSpaceKey == bRenderer::INPUT_PRESS){
				_running = !_running;
				bRenderer().getInput()->setCursorEnabled(!_running);
			}
		}

		// mouse look
		double xpos, ypos; bool hasCursor = false;
		bRenderer().getInput()->getCursorPosition(&xpos, &ypos, &hasCursor);

		deltaCameraY = (xpos - _mouseX) / 1000;
		_mouseX = xpos;
		deltaCameraX = (ypos - _mouseY) / 1000;
		_mouseY = ypos;

		if (_running){
			// movement using wasd keys
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_W) == bRenderer::INPUT_PRESS)
				if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT_SHIFT) == bRenderer::INPUT_PRESS) 			cameraForward = 4.0;
				else			cameraForward = 1.0;
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_S) == bRenderer::INPUT_PRESS)
				if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT_SHIFT) == bRenderer::INPUT_PRESS) 			cameraForward = -4.0;
				else			cameraForward = -1.0;
			else
				cameraForward = 0.0;

			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_A) == bRenderer::INPUT_PRESS)
				cameraSideward = -1.0;
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_D) == bRenderer::INPUT_PRESS)
				cameraSideward = 1.0;
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_UP) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->moveCameraUpward(_cameraSpeed*deltaTime);
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_DOWN) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->moveCameraUpward(-_cameraSpeed*deltaTime);
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->rotateCamera(0.0f, 0.0f, 0.03f*_cameraSpeed*deltaTime);
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_RIGHT) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->rotateCamera(0.0f, 0.0f, -0.03f*_cameraSpeed*deltaTime);
		}
	}

	//// Update camera ////
	if (_running){
		bRenderer().getObjects()->getCamera(camera)->moveCameraForward(cameraForward*_cameraSpeed*deltaTime);
		bRenderer().getObjects()->getCamera(camera)->rotateCamera(deltaCameraX, deltaCameraY, 0.0f);
		bRenderer().getObjects()->getCamera(camera)->moveCameraSideward(cameraSideward*_cameraSpeed*deltaTime);
	}	
}

/* For iOS only: Handle device rotation */
void RenderProject::deviceRotated()
{
	if (bRenderer().isInitialized()){
		// set view to full screen after device rotation
		bRenderer().getView()->setFullscreen(true);
		bRenderer::log("Device rotated");
	}
}

/* For iOS only: Handle app going into background */
void RenderProject::appWillResignActive()
{
	if (bRenderer().isInitialized()){
		// stop the renderer when the app isn't active
		bRenderer().stopRenderer();
	}
}

/* For iOS only: Handle app coming back from background */
void RenderProject::appDidBecomeActive()
{
	if (bRenderer().isInitialized()){
		// run the renderer as soon as the app is active
		bRenderer().runRenderer();
	}
}

/* For iOS only: Handle app being terminated */
void RenderProject::appWillTerminate()
{
	if (bRenderer().isInitialized()){
		// terminate renderer before the app is closed
		bRenderer().terminateRenderer();
	}
}

/* Helper functions */
GLfloat RenderProject::randomNumber(GLfloat min, GLfloat max){
	return min + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (max - min)));
}