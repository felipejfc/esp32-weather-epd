#include <vector>
#include <Arduino.h>

#include "alerts.h"
#include "api_response.h"
#include "util.h"

// extern to avoid redefintion of variables defined in config.h
extern const std::vector<String> alert_urgency;

/* Returns the urgency of an event based by checking if the event String
 * contains any indicator keywords.
 *
 * Urgency keywords are defined in config.h because they are very regional. 
 *   ex: United States - (Watch < Advisory < Warning)
 * 
 * The index in vector<String> alert_urgency indicates the urgency level.
 * If an event string matches none of these keywords the urgency is unknown, -1
 * is returned.
 * In the United States example, Watch = 0, Advisory = 1, Warning = 2
 */
int eventUrgency(String &event) {
  int urgency_lvl = -1;
  for (int i = 0; i < alert_urgency.size(); ++i) {
    if(event.indexOf(alert_urgency[i]) > 0) {
      urgency_lvl = i;
    }
  }
  return urgency_lvl;
}

/* This algorithm prepares alerts from the API responses to be displayed.
 *
 * Background:
 * The displaycan show up to 2 alerts, but alerts can be unpredictible in 
 * severity and number. If more than 2 alerts are active, this algorithm will 
 * attempt to interpret the urgency of each alert and prefer to display the most
 * urgent and recently issued alerts of each event type. Depending on the region
 * different keywords are used to convey the level of urgency.
 *
 * A vector array is used to store these keywords. (defined in config.h) Urgency
 * is ranked from low to high where the first index of the vector is the least 
 * urgent keyword and the last index is the most urgent keyword. Expected as all
 * lowercase.
 *
 * 
 * Pseudo Code:
 * Convert all event text and tags to lowercase.
 * 
 * // Deduplicate alerts of the same type
 * Dedup alerts with the same first tag. (ie. tag 0) Keeping only the most 
 *   urgent alerts of each tag and alerts who's urgency cannot be determined.
 * Note: urgency keywords are defined in config.h because they are very 
 *       regional. ex: United States - (Watch < Advisory < Warning)
 * 
 * // Save only the 2 most recent alerts
 * If (more than 2 weather alerts remain)
 *   Keep only the 2 most recently issued alerts (aka greatest "start" time)
 *   OpenWeatherMap provides this order, so we can just take index 0 and 1.
 * 
 * Truncate Extraneous Info (anything that follows a comma, period, or open 
 *   parentheses)
 * Convert all event text to Title Case. (Every Word Has Capital First Letter)
 */
void prepareAlerts(std::vector<owm_alerts_t> *resp) {
  // Convert all event text and tags to lowercase.
  for (auto alert : *resp) {
    alert.event.toLowerCase();
    alert.tags.toLowerCase();
  }

  // Deduplicate alerts with the same first tag. Keeping only the most urgent 
  // alerts of each tag and alerts who's urgency cannot be determined.
  for (auto it_a = resp->begin(); it_a != resp->end(); ++it_a) {
    if (it_a->tags.isEmpty()) {
      continue; // urgency can not be determined so it remains in the list
    }

    for (auto it_b = resp->begin(); it_b != resp->end(); ++it_b) {
      if (it_a != it_b && it_a->tags == it_b->tags) 
      {
        // comparing alerts of the same tag, removing the less urgent alert
        if (eventUrgency(it_a->event) >= eventUrgency(it_b->event)) {
          resp->erase(it_b);
        }
      }
    }

  }

  // Save only the 2 most recent alerts
  while (resp->size() > 2) {
    resp->pop_back();
  }

  // Prettify event strings
  for (auto alert : *resp) {
    truncateExtraneousInfo(&alert.event);
    toTitleCase(&alert.event);
  }

  return;

}