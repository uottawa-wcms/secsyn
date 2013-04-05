<?php

class SecureSyndicationMenuLinkProfile extends SecureSyndicationProfile {
  
  public function dependencies($object) {
    $deps = array();
    $menu = menu_load($object['menu_name']);
    $deps[] = array(
      'object' => $menu,
      'profile' => 'SecureSyndicationMenuProfile',
    );
    if (!empty($object['plid'])) {
      $parent_link = menu_link_load($object['plid']);
      $deps[] = array(
          'object' => $parent_link,
          'profile' => 'SecureSyndicationMenuLinkProfile',
      );
    }
    return $deps;
  }
  
  public function package($object) {
    $package = clone $object;
    unset($package['mlid']);
    if (!empty($package['plid'])) {
      $parent_link = menu_link_load($object['plid']);
      $package['parent_uuid'] = $this->uuid($parent_link);
    }
    unset($package['plid']);
    unset($package['router_path']);
    unset($package['has_children']);
    unset($package['depth']);
    for ($k = 1; $k <= 9; $k++) {
      unset($package['p' . $k]);
    }
    return $package;
  }
  
  public function update($uuid, $fields) {
    $old_item = $this->lookup($uuid);
    if (!empty($old_item)) {
      $fields['mlid'] = $old_item['mlid'];
    } else {
      $fields['mlid'] = NULL;
    }
    if (!empty($fields['parent_uuid'])) {
      $parent = $this->lookup($fields['parent_uuid']);
      $fields['plid'] = $parent->mlid;
      unset($fields['parent_uuid']);
    }
    return menu_link_save($fields, $old_item);
  }
  
  public function lookup($uuid) {
    return menu_uuid_load($uuid);
  }
  
  public function uuid($object) {
    return $object->uuid;
  }
  
}