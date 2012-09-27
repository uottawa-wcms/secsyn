<?php

class SecureSyndicationExportableProfile extends SecureSyndicationProfile {

  public function __construct() {
    ctools_include('export');
  }

  public function package($object) {
    $schema = $this->getSchema($object->table);
    $package = get_object_vars($object);
    unset($package[$schema['export']['primary key']]);
    return $package;
  }

  public function update($uuid, $fields) {
    $pieces = explode('/', $uuid);
    $existing = $this->lookup($uuid);
    if (!$existing) {
      $existing = ctools_export_crud_new($pieces[0], TRUE);
    }
    foreach ($fields as $key => $value) {
      $existing->$key = $fields[$value];
    }
    return ctools_export_crud_save($pieces[0], $fields);
  }

  public function lookup($uuid) {
    $pieces = explode('/', $uuid);
    return ctools_export_crud_load($pieces[0], $pieces[1]);
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
    return $object->table . '/' . $uuid_key;
  }

  private function getSchema($table) {
    return ctools_export_get_schema($table);
  }

}
/*
 *
  ctools_include('export');
  $tables = ctools_export_get_schemas();
  echo '<pre>';
  foreach ($tables as $table_name => $data) {
    echo $table_name . '<br />';
  }
  echo '</pre>';
 */
