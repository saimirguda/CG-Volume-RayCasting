#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include <GL/platform/Application.h>
#include <GL/platform/Window.h>

#include "Camera.h"
#include "InputHandler.h"
#include "OrbitalNavigator.h"
#include "Renderer.h"
#include "VolumeRenderer.h"

#include "framework/argparse.h"
#include "framework/png.h"

void printUsage(const char* argv0)
{
	std::cout << "usage: " << argv0 << " [options]\n";
	std::cout << "  --numthreads N     (use N threads for volume rendering)\n";
	std::cout << "  --configfile PATH  (use config-file specified by PATH)\n";
	std::cout << "  --output PATH      (create diff-files specified by PATH)\n";
	std::cout << "  --width N          (diff-mode rendered image width N)\n";
	std::cout << "  --height N         (diff-mode rendered image height N)\n";
}

int main(int argc, char* argv[])
{
	try
	{
		const char numthreads_token[] = "--numthreads";
		const char configfile_token[] = "--configfile";
		const char outputfile_token[] = "--output";
		const char width_token[] = "--width";
		const char height_token[] = "--height";

		int numthreads = static_cast<int>(std::thread::hardware_concurrency());
		std::string config_file = "";
		std::string output_file = "";
		int width = 800;
		int height = 600;

		for (char** a = &argv[1]; *a; ++a)
		{
			if (std::strcmp("--help", *a) == 0)
			{
				printUsage(argv[0]);
				return 0;
			}
			if (!argparse::checkArgument(numthreads_token, a, numthreads))
				if (!argparse::checkArgument(configfile_token, a, config_file))
					if (!argparse::checkArgument(outputfile_token, a, output_file))
						if (!argparse::checkArgument(width_token, a, width))
							if (!argparse::checkArgument(height_token, a, height))
								std::cout << "warning: unknown option " << *a
								          << " will be ignored" << std::endl;
		}

		if (output_file.empty())
		{
			// view mode
			OrbitalNavigator navigator(3.1415f / 2.f, 0.f, 100.f, math::float3(0.f, 0.f, 0.f));
			Camera camera(navigator, 1.f, 45.f, 800, 600);

			GL::platform::Window window("CG2 — Volume Rendering", 800, 600, 0, 0, false);
			Renderer renderer(window, camera, numthreads, config_file);
			InputHandler input_handler(navigator);

			window.attach(static_cast<GL::platform::KeyboardInputHandler*>(&input_handler));
			window.attach(static_cast<GL::platform::MouseInputHandler*>(&input_handler));

			GL::platform::run(renderer);
		}
		else
		{
			// diff mode
			OrbitalNavigator navigator(3.1415f / 2.f, 0.f, 100.f, math::float3(0.f, 0.f, 0.f));
			Camera camera(navigator, 1.f, 45.f, width, height);

			VolumeRenderer volume_renderer(camera, numthreads, config_file);
			volume_renderer.transfer_function.gui_auto_update_tf_lookup_table = false;
			if (volume_renderer.volume.isLoaded())
			{
				ColorBuffer texture;

				texture.resize(camera.getViewport().x, camera.getViewport().y);
				volume_renderer.cast_rays.resize(camera.getViewport().x, camera.getViewport().y);

				volume_renderer.renderBlocking(texture);
				texture.screenshot(output_file);
				volume_renderer.cast_rays.screenshot(output_file + "-cast_rays");

				volume_renderer.transfer_function.writeTransferLookupImage(output_file, 10);

				volume_renderer.writeUpSampledSlices(output_file, 10);

				volume_renderer.transfer_function.gui_req_update_tf_lookup_table = true;
				volume_renderer.transfer_function.updateLookupTable();
				volume_renderer.transfer_function.writeLookupTableImage(output_file);

				volume_renderer.writeAABB(output_file);
			}
			else
				std::cout << "ERROR: No volume loaded for Diff-Mode!" << std::endl;

			volume_renderer.shutdown();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "error: " << e.what() << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cout << "unknown exception" << std::endl;
		return -128;
	}
	return 0;
}
