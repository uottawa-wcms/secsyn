<?php

/**
 * Checks to see if the autosyndicator needs to be queued.
 *
 * Objects are queued if:
 * 1. The syndicator is enabled AND,
 * 2. The appropriate delay has passed
 *
 * @param object $syndicator An autosyndicator object
 * @return boolean TRUE if objects were queued, otherwise FALSE
 */
function secsyn_auto_check_queue($syndicator) {
  if ($syndicator->enabled != 'Y') {
    return FALSE;
  }
  if (!empty($syndicator->last_execution)) {
    $next = $syndicator->last_execution + $syndicator->execution_min_delay;
    if (time() < $next) {
      return FALSE;
    }
  }
  $filter = secsyn_filters_profile_load($syndicator->filter_name);
  secsyn_filters_queue_all($syndicator->remote_server_machine, $filter, $syndicator->urgent == 'Y');
  return TRUE;
}