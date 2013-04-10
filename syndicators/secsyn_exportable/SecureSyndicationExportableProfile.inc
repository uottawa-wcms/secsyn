<?php

class SecureSyndicationExportableProfile extends SecureSyndicationProfile {

  public function __construct() {
    ctools_include('export');
  }

  public function package($object) {
    $schema = $this->getSchema($object->table);
    unset($object->{$schema['export']['primary key']});
    unset($object->export_type);
    return $object;
  }

  public function update($uuid, $fields) {
    $pieces = explode('/', $uuid);
    $existing = $this->lookup($uuid);
    if ($existing) {
      $schema = $this->getSchema($pieces[0]);
      $fields->{$schema['export']['primary key']} = $existing->{$schema['export']['primary key']};
    }
    $fields = (object) (((array) $fields) + ((array) ctools_export_new_object($pieces[0])));
    $results = ctools_export_crud_save($pieces[0], $fields);
    return $results;
  }

  public function lookup($uuid) {
    $pieces = explode('/', $uuid);
    $item = ctools_export_crud_load($pieces[0], $pieces[1]);
    return $item;
  }

  public function uuid($object) {
    if (empty($object->table)) {
      return FALSE;
    }
    $schema = $this->getSchema($object->table);
    $uuid_key = $schema['export']['key'];
    if (empty($object->$uuid_key)) {
      return FALSE;
    }
    return $object->table . '/' . $object->$uuid_key;
  }

  private function getSchema($table) {
    return ctools_export_get_schema($table);
  }

}
