/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2016 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * sysload.c - System load monitor
 */

#define DEBUG_MODULE "SYSLOAD"
#include "sysload.h"

#include "FreeRTOS.h"
#include "debug.h"
#include "cfassert.h"
#include "param.h"
#include "static_mem.h"
#include "cmsis_os.h"

static void timerHandler(void *arg);
static bool isInit = false;
static uint8_t triggerDump = 0;
typedef struct {
  uint32_t ulRunTimeCounter;
  uint32_t xTaskNumber;
} taskData_t;

#define TASK_MAX_COUNT 32
NO_DMA_CCM_SAFE_ZERO_INIT static taskData_t previousSnapshot[TASK_MAX_COUNT];
static int taskTopIndex = 0;
static uint32_t previousTotalRunTime = 0;

static osTimerId_t timer;
static StaticTimer_t timerBuffer;

void sysLoadInit() {
  if (isInit)
		return;

	osTimerAttr_t timAttr = {
    .name = "sysLoad",
    .cb_mem = &timerBuffer,
    .cb_size = sizeof(timerBuffer),
  };

  timer = osTimerNew(timerHandler, osTimerPeriodic, NULL,  &timAttr);
  osTimerStart(timer, 1000);

  isInit = true;
}

bool sysLoadTest() {
	return isInit;
}

static taskData_t* getPreviousTaskData(uint32_t xTaskNumber) {
  // Try to find the task in the list of tasks
  for (int i = 0; i < taskTopIndex; i++) {
    if (previousSnapshot[i].xTaskNumber == xTaskNumber) {
      return &previousSnapshot[i];
    }
  }

  // Allocate a new entry
  ASSERT(taskTopIndex < TASK_MAX_COUNT);
  taskData_t *result = &previousSnapshot[taskTopIndex];
  result->xTaskNumber = xTaskNumber;

  taskTopIndex++;

  return result;
}

static void timerHandler(void *arg) {
  if (triggerDump != 0) {
    uint32_t totalRunTime;

    TaskStatus_t taskStats[TASK_MAX_COUNT];
    // TODO: check this
    uint32_t taskCount = uxTaskGetSystemState(taskStats, TASK_MAX_COUNT, &totalRunTime);
    ASSERT(taskCount < TASK_MAX_COUNT);

    uint32_t totalDelta = totalRunTime - previousTotalRunTime;
    float f = 100.0 / totalDelta;
    // Dumps the the CPU load and stack usage for all tasks
    // CPU usage is since last dump in % compared to total time spent in tasks. Note that time spent in interrupts will be included in measured time.
    // Stack usage is displayed as nr of unused bytes at peak stack usage.
    DEBUG_PRINT("Task dump\n");
    DEBUG_PRINT("Load\tStack left\tName\n");
    for (int i = 0; i < taskCount; i++) {
      TaskStatus_t* stats = &taskStats[i];
      taskData_t* previousTaskData = getPreviousTaskData(stats->xTaskNumber);

      uint32_t taskRunTime = stats->ulRunTimeCounter;
      float load = f * (taskRunTime - previousTaskData->ulRunTimeCounter);
      DEBUG_PRINT("%.2f \t%u \t%s\n", (double)load, stats->usStackHighWaterMark, stats->pcTaskName);
      previousTaskData->ulRunTimeCounter = taskRunTime;
    }

    previousTotalRunTime = totalRunTime;
    triggerDump = 0;
  }
}


PARAM_GROUP_START(system)

/**
 * @brief Set to nonzero to dump CPU and stack usage to console
 */
PARAM_ADD_CORE(PARAM_UINT8, taskDump, &triggerDump)

PARAM_GROUP_STOP(system)
