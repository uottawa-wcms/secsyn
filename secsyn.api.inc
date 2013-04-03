<?php

/**
 * Provides a list of profiles that this module implements.
 *
 * @return array An associative array where the key is the class name and the
 *   value is the human-readable name of the profile.
 */
function hook_secsyn_profiles() {

  return array(
    'SecureSyndicationEntityProfile' => t('Entities'),
  );
  
}

/**
 * Event trigger immediately before we syndicate on cron.
 */
function hook_secsyn_precron() {

  // do something like adding extra entities to the queue

}