<?php

class SecureSyndicationMenuProfile extends SecureSyndicationProfile {
  
  public function children($object) {
    $links = menu_load_links($object['menu_name']);
    $children = array();
    foreach ($links as $link) {
      $link = menu_link_load($object['mlid']);
      $children[] = array(
          'profile' => 'SecureSyndicationMenuLinkProfile',
          'object' => $link,
      );
    }
  }
  
  public function package($object) {
    return (array) $object;
  }
  
  public function update($uuid, $fields) {
    $fields['menu_name'] = $uuid;
    menu_save($fields);
  }
  
  public function lookup($uuid) {
    return menu_load($uuid);
  }
  
  public function uuid($object) {
    return $object->menu_name;
  }
  
}