/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor: 
 *		Jeroen Bakker 
 *		Monique Dewanchand
 */


#include "BKE_node.h"
extern "C" {
	#include "BLI_threads.h"
}
#include "BKE_main.h"
#include "BKE_global.h"

#include "COM_compositor.h"
#include "COM_ExecutionSystem.h"
#include "COM_WorkScheduler.h"
#include "OCL_opencl.h"

static ThreadMutex *compositorMutex;
void COM_execute(RenderData *rd, bNodeTree *editingtree, int rendering)
{
	if (compositorMutex == NULL) { /// TODO: move to blender startup phase
		compositorMutex = new ThreadMutex();
		BLI_mutex_init(compositorMutex);
		OCL_init();
		WorkScheduler::initialize(); ///TODO: call workscheduler.deinitialize somewhere
	}
	BLI_mutex_lock(compositorMutex);
	if (editingtree->test_break(editingtree->tbh)) {
		// during editing multiple calls to this method can be triggered.
		// make sure one the last one will be doing the work.
		BLI_mutex_unlock(compositorMutex);
		return;

	}


	/* set progress bar to 0% and status to init compositing*/
	editingtree->progress(editingtree->prh, 0.0);

	/* initialize execution system */
	Scene *scene;
	for (scene = (Scene*)G.main->scene.first; scene != NULL ; scene = (Scene*)scene->id.next) {
		if (&scene->r == rd) {
			ExecutionSystem *system = new ExecutionSystem(scene, editingtree, rendering);
			system->execute();
			delete system;
			break;
		}
	}

	BLI_mutex_unlock(compositorMutex);
}
