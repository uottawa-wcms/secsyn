<?php

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
}