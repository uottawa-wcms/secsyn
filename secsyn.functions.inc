<?php

function secsyn_get_profiles() {
  $profiles = &drupal_static(__FUNCTION__);
  if (empty($profiles)) {
    $profiles = cache_get('secsyn:profile_list');
    if (empty($profiles)) {
      $profiles = module_invoke_all('secsyn_profiles');
      cache_set('secsyn:profile_list', $profiles);
    }
  }
  return $profiles;
}