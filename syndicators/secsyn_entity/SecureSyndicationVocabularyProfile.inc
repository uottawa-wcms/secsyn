<?php

class SecureSyndicationVocabularyProfile extends SecureSyndicationProfile {

  public function package($object) {
    $package = clone $object;
    unset($package->vid);
    return $package;
  }

  public function update($uuid, $fields) {
    $existing = $this->lookup($uuid);
    if ($existing) {
      $fields->vid = $existing->vid;
    }
    return taxonomy_vocabulary_save($fields);
  }

  public function lookup($uuid) {
    return taxonomy_vocabulary_machine_name_load($uuid);
  }

  public function uuid($object) {
    return $object->machine_name;
  }

}
