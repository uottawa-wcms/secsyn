<?php

class SecureSyndicationExportableProfile extends SecureSyndicationProfile {

  public function __construct() {
    parent::__construct();
    ctools_include('export');
  }

  public function children($object) {
    $children = array();
    $instances = field_info_instances($object->entity_type, $object->bundle_name);
    foreach ($instances as $instance) {
      // TODO: Ignore certain fields
      $children[] = array(
        'profile' => 'SecureSyndicationFieldInstanceProfile',
        'object' => $instance,
      );
    }
    return $children;
  }

  public function package($object) {
    $schema = $this->getSchema($object->table);
    $package = (array) $object;
    unset($package[$schema['export']['primary key']]);
    return $package;
  }

  public function update($uuid, $fields) {
    $pieces = explode('/', $uuid);
    $existing = $this->lookup($uuid);
    if ($existing) {
      $schema = $this->getSchema($pieces[0]);
      $fields[$schema['export']['primary key']] = $existing[$schema['export']['primary key']];
    }
    $fields = (object) $fields;
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
