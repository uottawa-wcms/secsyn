<?php

function secsyn_get_profiles() {
  $profiles = &drupal_static(__FUNCTION__);
  if (empty($profiles)) {
    $profiles = module_invoke_all('secsyn_profiles');
  }
  return $profiles;
}

function secsyn_syndicate_all($from_cron = FALSE) {
  $start_time = time();
  $elapsed_time = 0;
  $max_time = 60;
  $mode = 0;
  $urgent_only = TRUE;
  $hour = date('s') + (date('i') * 60) + (date('G') * 3600);
  $syndication_blackout_start = 0;
  $syndication_blackout_end = 0;
  // if we're outside of the blackout period, syndicate only essential things
  if ($syndication_blackout_start != $syndication_blackout_end) {
    if ($hour < $syndication_blackout_start || $hour > $syndication_blackout_end) {
      $urgent_only = FALSE;
    }
  } else {
    // if they are equal, there is no blackout period
    $urgent_only = FALSE;
  }
  $modes = array(
    'checkQueue',
    'checkNextBatch',
    'syndicateNextBatch',
  );
  while (($mode < count($modes)) && ($elapsed_time < $max_time)) {
    $engine = new SecureSyndicationEngine();
    $current_operation = $modes[$mode];
    $result = $engine->$current_operation($urgent_only);
    if (empty($result)) {
      $mode++;
    }
    $elapsed_time = time() - $start_time;
  }
}